# Copyright 2022 Cartesi Pte. Ltd.
#
# SPDX-License-Identifier: Apache-2.0
# Licensed under the Apache License, Version 2.0 (the "License"); you may not use
# this file except in compliance with the License. You may obtain a copy of the
# License at http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software distributed
# under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
# CONDITIONS OF ANY KIND, either express or implied. See the License for the
# specific language governing permissions and limitations under the License.

CC := riscv64-cartesi-linux-gnu-gcc
CC_HOST := gcc

.PHONY: clean 3rdparty

echo-backend: echo-backend.c
	make -C 3rdparty
	$(CC) echo-backend.c -o echo

echo-backend-host: echo-backend.c
	make -C 3rdparty
	$(CC_HOST) echo-backend.c -o echo

clean:
	@rm -rf echo-backend echo-backend-host
	make -C 3rdparty clean