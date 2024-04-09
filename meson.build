# SPDX-License-Identifier: BSD-3-Clause

project('qrtr',
        'c',
        license : [ 'BSD-3-Clause'],
        default_options : [
        'warning_level=1',
        'buildtype=release',
        ])

prefix = get_option('prefix')
systemd = dependency('systemd')

systemd_system_unit_dir = get_option('systemd-unit-prefix')
if systemd_system_unit_dir == ''
        systemd_system_unit_dir = systemd.get_variable(
                pkgconfig : 'systemdsystemunitdir',
                pkgconfig_define: ['prefix', prefix])
endif

inc = include_directories('include')
subdir('lib')
subdir('include')
subdir('src')

systemd_unit_conf = configuration_data()
systemd_unit_conf.set('prefix', prefix)
configure_file(
               input : 'qrtr-ns.service.in',
               output : 'qrtr-ns.service',
               configuration : systemd_unit_conf,
               install_dir : systemd_system_unit_dir)
