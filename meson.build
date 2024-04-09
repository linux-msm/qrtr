# SPDX-License-Identifier: BSD-3-Clause

project('qrtr',
        'c',
        license : [ 'BSD-3-Clause'],
        default_options : [
        'warning_level=1',
        'buildtype=release',
        ])

prefix = get_option('prefix')
with_qrtr_ns = get_option('qrtr-ns')

install_systemd_unit = get_option('systemd-service')
systemd = dependency('systemd', required : install_systemd_unit)
if systemd.found()
        systemd_system_unit_dir = get_option('systemd-unit-prefix')
        if systemd_system_unit_dir == ''
                systemd_system_unit_dir = systemd.get_variable(
                        pkgconfig : 'systemdsystemunitdir',
                pkgconfig_define: ['prefix', prefix])
        else
                message('Could not resolve systemd dependencies, skipping unit file')
                install_systemd_unit = false
        endif
endif

inc = include_directories('include')
subdir('lib')
subdir('include')
subdir('src')

if systemd.found() and with_qrtr_ns.enabled()
        systemd_unit_conf = configuration_data()
        systemd_unit_conf.set('prefix', prefix)
        configure_file(
                input : 'qrtr-ns.service.in',
                output : 'qrtr-ns.service',
                configuration : systemd_unit_conf,
                install_dir : systemd_system_unit_dir)
endif
