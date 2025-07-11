#
# Copyright(c) 2019 to 2021 ZettaScale Technology and others
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License v. 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
# v. 1.0 which is available at
# http://www.eclipse.org/org/documents/edl-v10.php.
#
# SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
#

# Both executables have only one related source file.
add_executable(RoundtripPing ping.c RoundTrip.c)
add_executable(RoundtripPong pong.c RoundTrip.c)

# Both executables need to be linked to the idl data type library and
# the ddsc API library.
target_link_libraries(RoundtripPing ddscxx ddsc)
target_link_libraries(RoundtripPong ddscxx ddsc)
