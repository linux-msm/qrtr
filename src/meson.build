# SPDX-License-Identifier: BSD-3-Clause

cfg_srcs = ['addr.c',
            'cfg.c',
            'hash.c']
executable('qrtr-cfg',
           cfg_srcs,
           link_with : libqrtr,
           include_directories : inc,
           install : true)

ns_srcs = ['addr.c',
           'hash.c',
           'map.c',
           'ns.c',
           'util.c',
           'waiter.c']
executable('qrtr-ns',
           ns_srcs,
           link_with : libqrtr,
           include_directories : inc,
           install : true)

executable('qrtr-lookup',
           'lookup.c',
           link_with : libqrtr,
           include_directories : inc,
           install : true)