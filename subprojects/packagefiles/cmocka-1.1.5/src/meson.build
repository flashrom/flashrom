# Copyright © 2018 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# cmocka rather annoyingly uses different standards for different platforms.
_overrides = []
if host_machine.system() != 'windows'
  _overrides += 'c_std=gnu99'
endif

libcmocka = library(
  'cmocka',
  ['cmocka.c'],
  c_args : '-DHAVE_CONFIG_H=1',
  include_directories : [inc_include, inc_private],
  vs_module_defs : 'cmocka.def',
  soversion : host_machine.system() != 'windows' ? lib_version.split('.')[0] : '',
  version : lib_version,
  install : false,
  override_options : _overrides,
  build_by_default: false,
)

libcmocka_native = library(
  'cmocka_native',
  ['cmocka.c'],
  c_args : '-DHAVE_CONFIG_H=1',
  include_directories : [inc_include, inc_private_native],
  vs_module_defs : 'cmocka.def',
  soversion : build_machine.system() != 'windows' ? lib_version.split('.')[0] : '',
  version : lib_version,
  install : false, # Cannot install native targets in cross builds
  override_options : _overrides,
  native: true,
  build_by_default: false,
)
