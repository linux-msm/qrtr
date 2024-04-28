# SPDX-License-Identifier: BSD-3-Clause

pkg = import('pkgconfig')

libqrtr_srcs = ['logging.c', 'qmi.c', 'qrtr.c']
libqrtr = shared_library('qrtr',
                         libqrtr_srcs,
                         include_directories : inc,
                         install : true,
                         version : meson.project_version())

pkg.generate(libqrtr)
