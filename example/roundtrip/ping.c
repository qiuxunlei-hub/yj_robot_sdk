#include "dds/dds.h"
#include "dds/ddsrt/misc.h"
#include "RoundTrip.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <inttypes.h>

#define TIME_STATS_SIZE_INCREMENT 50000     // 时间统计数组增量大小
#define MAX_SAMPLES 100                     // 最大样本数
#define US_IN_ONE_SEC 1000000LL             // 1秒的微秒数

/* Forward declaration */
static dds_entity_t prepare_dds(dds_entity_t *writer, dds_entity_t *reader, dds_entity_t *readCond, dds_listener_t *listener);
static void finalize_dds(dds_entity_t participant);

/* 时间统计数据结构 */
typedef struct ExampleTimeStats {
  dds_time_t * values;        // 存储时间值的数组
  unsigned long valuesSize;   // 当前存储值的数量
  unsigned long valuesMax;    // 数组最大容量
  double average;             // 平均值
  dds_time_t min;             // 最小值
  dds_time_t max;             // 最大值
  unsigned long count;        // 计数器
} ExampleTimeStats;

/* 初始化时间统计结构 */
static void exampleInitTimeStats (ExampleTimeStats *stats)
{
  stats->values = (dds_time_t*) malloc (TIME_STATS_SIZE_INCREMENT * sizeof (dds_time_t));
  stats->valuesSize = 0;
  stats->valuesMax = TIME_STATS_SIZE_INCREMENT;
  stats->average = 0;
  stats->min = 0;
  stats->max = 0;
  stats->count = 0;
}

/* 重置时间统计数据 */
static void exampleResetTimeStats (ExampleTimeStats *stats)
{
  memset (stats->values, 0, stats->valuesMax * sizeof (dds_time_t));
  stats->valuesSize = 0;
  stats->average = 0;
  stats->min = 0;
  stats->max = 0;
  stats->count = 0;
}

/* 释放时间统计结构 */
static void exampleDeleteTimeStats (ExampleTimeStats *stats)
{
  free (stats->values);
}

/* 添加时间值到统计结构 */
static ExampleTimeStats *exampleAddTimingToTimeStats(ExampleTimeStats *stats, dds_time_t timing)
{
  // 检查是否需要扩展数组
  if (stats->valuesSize > stats->valuesMax)
  {
    dds_time_t * temp = (dds_time_t*) realloc (stats->values, (stats->valuesMax + TIME_STATS_SIZE_INCREMENT) * sizeof (dds_time_t));
    stats->values = temp;
    stats->valuesMax += TIME_STATS_SIZE_INCREMENT;
  }

  // 存储时间值
  if (stats->values != NULL && stats->valuesSize < stats->valuesMax)
  {
    stats->values[stats->valuesSize++] = timing;
  }

  // 更新统计值
  stats->average = ((double)stats->count * stats->average + (double)timing) / (double)(stats->count + 1);
  stats->min = (stats->count == 0 || timing < stats->min) ? timing : stats->min;
  stats->max = (stats->count == 0 || timing > stats->max) ? timing : stats->max;
  stats->count++;

  return stats;
}

/* 比较函数用于qsort排序 */
static int exampleCompareul (const void* a, const void* b)
{
  dds_time_t ul_a = *((dds_time_t*)a);
  dds_time_t ul_b = *((dds_time_t*)b);

  if (ul_a < ul_b) return -1;
  if (ul_a > ul_b) return 1;
  return 0;
}

/* 计算中值 */
static double exampleGetMedianFromTimeStats (ExampleTimeStats *stats)
{
  double median = 0.0;

  qsort (stats->values, stats->valuesSize, sizeof (dds_time_t), exampleCompareul);

  if (stats->valuesSize % 2 == 0)
  {
    median = (double)(stats->values[stats->valuesSize / 2 - 1] + stats->values[stats->valuesSize / 2]) / 2;
  }
  else
  {
    median = (double)stats->values[stats->valuesSize / 2];
  }

  return median;
}

/* 计算99%分位数 */
static dds_time_t exampleGet99PercentileFromTimeStats (ExampleTimeStats *stats)
{
  qsort (stats->values, stats->valuesSize, sizeof (dds_time_t), exampleCompareul);
  return stats->values[stats->valuesSize - stats->valuesSize / 100];
}

static dds_entity_t waitSet;

#ifdef _WIN32
#include <Windows.h>
static bool CtrlHandler (DWORD fdwCtrlType)
{
  (void)fdwCtrlType;
  dds_waitset_set_trigger (waitSet, true);
  return true; //Don't let other handlers handle this key
}
#elif !DDSRT_WITH_FREERTOS
static void CtrlHandler (int sig)
{
  (void)sig;
  dds_waitset_set_trigger (waitSet, true);
}
#endif

static dds_entity_t writer;           // 数据写入器
static dds_entity_t reader;           // 数据读取器
static dds_entity_t participant;      // 域参与者
static dds_entity_t readCond;         // 读取条件

/* 时间统计结构实例 */
static ExampleTimeStats roundTrip;         // 往返时间统计
static ExampleTimeStats writeAccess;       // 写入时间统计
static ExampleTimeStats readAccess;        // 读取时间统计
static ExampleTimeStats roundTripOverall;  // 总体往返时间统计
static ExampleTimeStats writeAccessOverall;// 总体写入时间统计
static ExampleTimeStats readAccessOverall; // 总体读取时间统计

static RoundTripModule_DataType pub_data;          // 发布数据
static RoundTripModule_DataType sub_data[MAX_SAMPLES]; // 订阅数据数组
static void *samples[MAX_SAMPLES];                 // 样本指针数组
static dds_sample_info_t info[MAX_SAMPLES];        // 样本信息数组

/* 时间戳变量 */
static dds_time_t startTime;      // 开始时间
static dds_time_t preWriteTime;   // 写入前时间
static dds_time_t postWriteTime;  // 写入后时间
static dds_time_t preTakeTime;    // 读取前时间
static dds_time_t postTakeTime;   // 读取后时间
static dds_time_t elapsed = 0;    // 已过去时间

static bool warmUp = true;        // 预热标志

/* 数据到达回调函数 */
static void data_available(dds_entity_t rd, void *arg)
{
  dds_time_t difference = 0;
  int status;
  (void)arg;
  /* Take sample and check that it is valid */
  preTakeTime = dds_time ();
  status = dds_take (rd, samples, info, MAX_SAMPLES, MAX_SAMPLES);
  if (status < 0)
    DDS_FATAL("dds_take: %s\n", dds_strretcode(-status));
  postTakeTime = dds_time ();

  /* Update stats */
  difference = (postWriteTime - preWriteTime)/DDS_NSECS_IN_USEC;
  writeAccess = *exampleAddTimingToTimeStats (&writeAccess, difference);
  writeAccessOverall = *exampleAddTimingToTimeStats (&writeAccessOverall, difference);

  difference = (postTakeTime - preTakeTime)/DDS_NSECS_IN_USEC;
  readAccess = *exampleAddTimingToTimeStats (&readAccess, difference);
  readAccessOverall = *exampleAddTimingToTimeStats (&readAccessOverall, difference);

  difference = (postTakeTime - info[0].source_timestamp)/DDS_NSECS_IN_USEC;
  roundTrip = *exampleAddTimingToTimeStats (&roundTrip, difference);
  roundTripOverall = *exampleAddTimingToTimeStats (&roundTripOverall, difference);

  if (!warmUp) {
    /* Print stats each second */
    difference = (postTakeTime - startTime)/DDS_NSECS_IN_USEC;
    if (difference > US_IN_ONE_SEC)
    {
      printf("%9" PRIi64 " %9lu %8.0f %8" PRIi64 " %8" PRIi64 " %8" PRIi64 " %10lu %8.0f %8" PRIi64 " %10lu %8.0f %8" PRIi64 "\n",
             elapsed + 1,
             roundTrip.count,
             exampleGetMedianFromTimeStats (&roundTrip) / 2,
             roundTrip.min / 2,
             exampleGet99PercentileFromTimeStats (&roundTrip) / 2,
             roundTrip.max / 2,
             writeAccess.count,
             exampleGetMedianFromTimeStats (&writeAccess),
             writeAccess.min,
             readAccess.count,
             exampleGetMedianFromTimeStats (&readAccess),
             readAccess.min);
      fflush (stdout);

      exampleResetTimeStats (&roundTrip);
      exampleResetTimeStats (&writeAccess);
      exampleResetTimeStats (&readAccess);
      startTime = dds_time ();
      elapsed++;
    }
  }

  preWriteTime = dds_time();
  status = dds_write_ts (writer, &pub_data, preWriteTime);
  if (status < 0)
    DDS_FATAL("dds_write_ts: %s\n", dds_strretcode(-status));
  postWriteTime = dds_time();
}

/* 使用说明 */
static void usage(void)
{
  printf ("Usage (parameters must be supplied in order):\n"
          "./ping [-l] [payloadSize (bytes, 0 - 100M)] [numSamples (0 = infinite)] [timeOut (seconds, 0 = infinite)]\n"
          "./ping quit - ping sends a quit signal to pong.\n"
          "Defaults:\n"
          "./ping 0 0 0\n");
  exit(EXIT_FAILURE);
}

/* 主函数 */
int main (int argc, char *argv[])
{
  uint32_t payloadSize = 0;      // 负载大小
  uint64_t numSamples = 0;       // 样本数量
  bool invalidargs = false;      // 无效参数标志
  dds_time_t timeOut = 0;        // 超时时间
  dds_time_t time;               // 当前时间
  dds_time_t difference = 0;     // 时间差

  dds_attach_t wsresults[1];     // 等待集结果
  size_t wsresultsize = 1U;      // 等待集结果大小
  dds_time_t waitTimeout = DDS_SECS(1); // 等待超时时间
  unsigned long i;               // 循环变量
  int status;                    // 状态码

  dds_listener_t *listener = NULL; // 监听器
  bool use_listener = false;     // 是否使用监听器
  int argidx = 1;                // 参数索引

  /* 解析参数 */
  if (argc > argidx && strcmp(argv[argidx], "-l") == 0)
  {
    argidx++;
    use_listener = true;
  }

  /* Register handler for Ctrl-C */
#ifdef _WIN32
  DDSRT_WARNING_GNUC_OFF(cast-function-type)
  SetConsoleCtrlHandler ((PHANDLER_ROUTINE)CtrlHandler, TRUE);
  DDSRT_WARNING_GNUC_ON(cast-function-type)
#elif !DDSRT_WITH_FREERTOS
  struct sigaction sat, oldAction;
  sat.sa_handler = CtrlHandler;
  sigemptyset (&sat.sa_mask);
  sat.sa_flags = 0;
  sigaction (SIGINT, &sat, &oldAction);
#endif

  /* 初始化统计结构 */
  exampleInitTimeStats (&roundTrip);
  exampleInitTimeStats (&writeAccess);
  exampleInitTimeStats (&readAccess);
  exampleInitTimeStats (&roundTripOverall);
  exampleInitTimeStats (&writeAccessOverall);
  exampleInitTimeStats (&readAccessOverall);

  /* 初始化数据样本 */
  memset (&sub_data, 0, sizeof (sub_data));
  memset (&pub_data, 0, sizeof (pub_data));

  for (i = 0; i < MAX_SAMPLES; i++)
  {
    samples[i] = &sub_data[i];
  }

  /* 创建域参与者 */
  participant = dds_create_participant (DDS_DOMAIN_DEFAULT, NULL, NULL);
  if (participant < 0)
    DDS_FATAL("dds_create_participant: %s\n", dds_strretcode(-participant));

  /* 设置监听器 */
  if (use_listener)
  {
    listener = dds_create_listener(NULL);
    dds_lset_data_available(listener, data_available);
  }

  /* 准备DDS实体 */
  prepare_dds(&writer, &reader, &readCond, listener);

  /* 处理quit命令 */
  if (argc - argidx == 1 && strcmp (argv[argidx], "quit") == 0)
  {
    printf ("Sending termination request.\n");
    fflush (stdout);
    /* pong uses a waitset which is triggered by instance disposal, and
      quits when it fires. */
    dds_sleepfor (DDS_SECS (1));

    // 准备空负载
    pub_data.payload._length = 0;
    pub_data.payload._buffer = NULL;
    pub_data.payload._release = true;
    pub_data.payload._maximum = 0;

    // 写入并处置样本
    status = dds_writedispose (writer, &pub_data);
    if (status < 0)
      DDS_FATAL("dds_writedispose: %s\n", dds_strretcode(-status));
    dds_sleepfor (DDS_SECS (1));
    goto done;
  }

  /* 解析其他参数 */
  if (argc - argidx == 0)
  {
    invalidargs = true;
  }
  if (argc - argidx >= 1)
  {
    payloadSize = (uint32_t) atol (argv[argidx]);

    if (payloadSize > 100 * 1048576)
    {
      invalidargs = true;
    }
  }

  if (argc - argidx >= 2)
  {
    numSamples = (uint64_t) atol (argv[argidx+1]);
  }
  if (argc - argidx >= 3)
  {
    timeOut = atol (argv[argidx+2]);
  }

  /* 检查参数有效性 */
  if (invalidargs || (argc - argidx == 1 && (strcmp (argv[argidx], "-h") == 0 || strcmp (argv[argidx], "--help") == 0)))
    usage();
  printf ("# payloadSize: %" PRIu32 " | numSamples: %" PRIu64 " | timeOut: %" PRIi64 "\n\n", payloadSize, numSamples, timeOut);
  fflush (stdout);

  /* 准备发布数据 */
  pub_data.payload._length = payloadSize;
  pub_data.payload._buffer = payloadSize ? dds_alloc (payloadSize) : NULL;
  pub_data.payload._release = true;
  pub_data.payload._maximum = 0;

  // 填充负载数据
  for (i = 0; i < payloadSize; i++)
  {
    pub_data.payload._buffer[i] = 'a';
  }

  /* 预热阶段 */
  startTime = dds_time ();
  printf ("# Waiting for startup jitter to stabilise\n");
  fflush (stdout);
  /* Write a sample that pong can send back */
  while (!dds_triggered (waitSet) && difference < DDS_SECS(5))
  {
    status = dds_waitset_wait (waitSet, wsresults, wsresultsize, waitTimeout);
    if (status < 0)
      DDS_FATAL("dds_waitset_wait: %s\n", dds_strretcode(-status));

    if (status > 0 && listener == NULL) /* data */
    {
      status = dds_take (reader, samples, info, MAX_SAMPLES, MAX_SAMPLES);
      if (status < 0)
        DDS_FATAL("dds_take: %s\n", dds_strretcode(-status));
    }

    time = dds_time ();
    difference = time - startTime;
  }

  /* 预热完成，开始正式测试 */
  if (!dds_triggered (waitSet))
  {
    warmUp = false;
    printf("# Warm up complete.\n\n");
    printf("# Latency measurements (in us)\n");
    printf("#             Latency [us]                                   Write-access time [us]       Read-access time [us]\n");
    printf("# Seconds     Count   median      min      99%%      max      Count   median      min      Count   median      min\n");
    fflush (stdout);
  }

  exampleResetTimeStats (&roundTrip);
  exampleResetTimeStats (&writeAccess);
  exampleResetTimeStats (&readAccess);
  startTime = dds_time ();
  /* Write a sample that pong can send back */
  preWriteTime = dds_time ();
  status = dds_write_ts (writer, &pub_data, preWriteTime);
  if (status < 0)
    DDS_FATAL("dds_write_ts: %s\n", dds_strretcode(-status));
  postWriteTime = dds_time ();
  for (i = 0; !dds_triggered (waitSet) && (!numSamples || i < numSamples) && !(timeOut && elapsed >= timeOut); i++)
  {
    status = dds_waitset_wait (waitSet, wsresults, wsresultsize, waitTimeout);
    if (status < 0)
      DDS_FATAL("dds_waitset_wait: %s\n", dds_strretcode(-status));
    if (status != 0 && listener == NULL) {
      data_available(reader, NULL);
    }
  }

  if (!warmUp)
  {
    printf
    (
      "\n%9s %9lu %8.0f %8" PRIi64 " %8" PRIi64 " %8" PRIi64 " %10lu %8.0f %8" PRIi64 " %10lu %8.0f %8" PRIi64 "\n",
      "# Overall",
      roundTripOverall.count,
      exampleGetMedianFromTimeStats (&roundTripOverall) / 2,
      roundTripOverall.min / 2,
      exampleGet99PercentileFromTimeStats (&roundTripOverall) / 2,
      roundTripOverall.max / 2,
      writeAccessOverall.count,
      exampleGetMedianFromTimeStats (&writeAccessOverall),
      writeAccessOverall.min,
      readAccessOverall.count,
      exampleGetMedianFromTimeStats (&readAccessOverall),
      readAccessOverall.min
    );
    fflush (stdout);
  }

done:

#ifdef _WIN32
  SetConsoleCtrlHandler (0, FALSE);
#elif !DDSRT_WITH_FREERTOS
  sigaction (SIGINT, &oldAction, 0);
#endif

  finalize_dds(participant);

  /* Clean up */
  exampleDeleteTimeStats (&roundTrip);
  exampleDeleteTimeStats (&writeAccess);
  exampleDeleteTimeStats (&readAccess);
  exampleDeleteTimeStats (&roundTripOverall);
  exampleDeleteTimeStats (&writeAccessOverall);
  exampleDeleteTimeStats (&readAccessOverall);

  for (i = 0; i < MAX_SAMPLES; i++)
  {
    RoundTripModule_DataType_free (&sub_data[i], DDS_FREE_CONTENTS);
  }
  RoundTripModule_DataType_free (&pub_data, DDS_FREE_CONTENTS);

  return EXIT_SUCCESS;
}

/* 准备DDS实体函数 */
static dds_entity_t prepare_dds(dds_entity_t *wr, dds_entity_t *rd, dds_entity_t *rdcond, dds_listener_t *listener)
{
  dds_return_t status;
  dds_entity_t topic;
  dds_entity_t publisher;
  dds_entity_t subscriber;

  const char *pubPartitions[] = { "ping" };
  const char *subPartitions[] = { "pong" };
  dds_qos_t *pubQos;
  dds_qos_t *dwQos;
  dds_qos_t *drQos;
  dds_qos_t *subQos;

  /* 创建主题 */
  topic = dds_create_topic (participant, &RoundTripModule_DataType_desc, "RoundTrip", NULL, NULL);
  if (topic < 0)
    DDS_FATAL("dds_create_topic: %s\n", dds_strretcode(-topic));

  /* 创建发布者 */
  pubQos = dds_create_qos ();
  dds_qset_partition (pubQos, 1, pubPartitions);

  publisher = dds_create_publisher (participant, pubQos, NULL);

  if (publisher < 0)
    DDS_FATAL("dds_create_publisher: %s\n", dds_strretcode(-publisher));
  dds_delete_qos (pubQos);

  /* 创建数据写入器 */
  dwQos = dds_create_qos ();
  dds_qset_reliability (dwQos, DDS_RELIABILITY_RELIABLE, DDS_SECS (10));
  dds_qset_writer_data_lifecycle (dwQos, false);
  *wr = dds_create_writer (publisher, topic, dwQos, NULL);
  
  if (*wr < 0)
    DDS_FATAL("dds_create_writer: %s\n", dds_strretcode(-*wr));
  dds_delete_qos (dwQos);

  /* 创建订阅者 */
  subQos = dds_create_qos ();

  dds_qset_partition (subQos, 1, subPartitions);

  subscriber = dds_create_subscriber (participant, subQos, NULL);
  if (subscriber < 0)
    DDS_FATAL("dds_create_subscriber: %s\n", dds_strretcode(-subscriber));
  dds_delete_qos (subQos);

  /* 创建数据读取器 */
  drQos = dds_create_qos ();
  dds_qset_reliability (drQos, DDS_RELIABILITY_RELIABLE, DDS_SECS(10));
  *rd = dds_create_reader (subscriber, topic, drQos, listener);
  if (*rd < 0)
    DDS_FATAL("dds_create_reader: %s\n", dds_strretcode(-*rd));
  dds_delete_qos (drQos);

  /* 创建等待集 */
  waitSet = dds_create_waitset (participant);

  /* 如果未使用监听器，创建读取条件并附加到等待集 */
  if (listener == NULL) {
    *rdcond = dds_create_readcondition (*rd, DDS_ANY_STATE);
    status = dds_waitset_attach (waitSet, *rdcond, *rd);
    if (status < 0)
      DDS_FATAL("dds_waitset_attach: %s\n", dds_strretcode(-status));
  } else {
    *rdcond = 0;
  }

  /* 将等待集自身附加到等待集 */
  status = dds_waitset_attach (waitSet, waitSet, waitSet);
  if (status < 0)
    DDS_FATAL("dds_waitset_attach: %s\n", dds_strretcode(-status));

  return participant;
}

/* 清理DDS实体函数 */
static void finalize_dds(dds_entity_t ppant)
{
  dds_return_t status;
  status = dds_delete (ppant);
  if (status < 0)
    DDS_FATAL("dds_delete: %s\n", dds_strretcode(-status));
}
