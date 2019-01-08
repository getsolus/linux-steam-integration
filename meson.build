project(
    'linux-steam-integration',
    ['c'],
    version: '0.7.2',
    license: [
        'LGPL-2.1',
    ],
    default_options: [
        'c_std=c11',
    ],
)

am_cflags = [
    '-fstack-protector',
    '-pedantic',
    '-Wstrict-prototypes',
    '-Wundef',
    '-fno-common',
    '-Werror-implicit-function-declaration',
    '-Wformat',
    '-Wformat-security',
    '-Werror=format-security',
    '-Wno-conversion',
    '-Wunused-variable',
    '-Wunreachable-code',
    '-Wall',
    '-W',
    '-D_FORTIFY_SOURCE=2',
]

add_global_arguments(am_cflags, language: 'c')

dep_gtk3 = dependency('gtk+-3.0', version: '>= 3.4.0')

# Grab all our main paths
path_prefix = get_option('prefix')
path_bindir = join_paths(path_prefix, get_option('bindir'))
path_datadir = join_paths(path_prefix, get_option('datadir'))
path_vendordir = join_paths(path_datadir, 'defaults', meson.project_name())
path_sysconfdir = join_paths(path_prefix, get_option('sysconfdir'))
path_localedir = join_paths(path_prefix, get_option('localedir'))

# Set up our basic configuration
cdata = configuration_data()
cdata.set_quoted('PACKAGE_NAME', meson.project_name())
cdata.set_quoted('SYSTEMCONFDIR', path_sysconfdir)
cdata.set_quoted('VENDORDIR', path_vendordir)
cdata.set_quoted('LOCALEDIR', path_localedir)
cdata.set_quoted('GETTEXT_PACKAGE', meson.project_name())

# Configurables
cdata.set_quoted('LSI_PRELOAD_LIBS', get_option('with-preload-libs'))
cdata.set_quoted('STEAM_BINARY', get_option('with-steam-binary'))

# Future reports
shim_system_report = []
intercept_module_report = []
redirect_module_report = []

# Control the shim behaviour
shim_behaviour = get_option('with-shim')
shim_enabled = false
if shim_behaviour == 'replacement'
    cdata.set('REPLACE_STEAM', '1')
endif
if shim_behaviour != 'none'
    shim_enabled = true
endif
shim_system_report += '    behaviour:                              @0@'.format(shim_behaviour)

# Do we have liblsi-intercept?
with_libintercept = get_option('with-libintercept')
if with_libintercept == true
    cdata.set('HAVE_LIBINTERCEPT', '1')
endif

# Do we have liblsi-redirect?
with_libredirect = get_option('with-libredirect')
if with_libredirect == true
    cdata.set('HAVE_LIBREDIRECT', '1')
endif

# Do we have the frontend?
with_frontend = get_option('with-frontend')

# Using the new libc++ ABI? Important for mesa drivers
with_new_libcxx_abi = get_option('with-new-libcxx-abi')
if with_new_libcxx_abi == true
    cdata.set('LSI_USE_NEW_ABI', '1')
endif
shim_system_report += '    New C++ ABI:                            @0@'.format(with_new_libcxx_abi)

# How are we handling LibreSSL?
with_libressl_mode = get_option('with-libressl-mode')
intercept_module_report += '    LibreSSL handling:                      @0@'.format(with_libressl_mode)

if with_libressl_mode == 'native'
    # LibreSSL is provided by the distro as the native libssl, libcrypto, etc
    cdata.set('LSI_LIBRESSL_MODE_NATIVE', '1')
    cdata.set('LSI_OVERRIDE_LIBRESSL', '1')
elif with_libressl_mode == 'shim'
    # A shim package has been provided for soname redirects to updated/secure libressl
    # This allows distros to secure libressl without making it the system libssl/libcrypto option
    cdata.set('LSI_LIBRESSL_MODE_SHIM', '1')
    cdata.set('LSI_OVERRIDE_LIBRESSL', '1')
    with_libressl_suffix = get_option('with-libressl-suffix')
    intercept_module_report += '    LibreSSL library suffix:                @0@'.format(with_libressl_suffix)
    cdata.set_quoted('LSI_LIBRESSL_SUFFIX', with_libressl_suffix)
else
    message('LibreSSL mode isn\'t configured which may impact security')
endif

# Got snap support?
with_snap_support = get_option('with-snap-support')
if with_snap_support == true
    cdata.set('HAVE_SNAPD_SUPPORT', '1')
    path_fake_scripts_dir = join_paths(path_datadir, meson.project_name(), 'scripts')
    cdata.set_quoted('FAKE_SCRIPTS_DIR', path_fake_scripts_dir)
    subdir('snapd')
    message('Enabling snapd support. Do NOT do this for distribution builds!')
endif

# Let's write the global config.h now
config_h = configure_file(
     configuration: cdata,
     output: 'config.h',
)
# Make it discoverable
config_h_dir = include_directories('.')

# Descend into source trees
subdir('data')
subdir('src')
subdir('po')

report = [
    '    Build configuration:',
    '    ====================',
    '',
    '    prefix:                                 @0@'.format(path_prefix),
    '    datadir:                                @0@'.format(path_datadir),
    '    sysconfdir:                             @0@'.format(path_sysconfdir),
    '    bindir:                                 @0@'.format(path_bindir),
    '',
    '    Shim System (Execution/Environment Control):',
    '    ============================================',
    '',
    '    enabled:                                @0@'.format(shim_enabled),
    '@0@'.format('\n'.join(shim_system_report)),
    '',
    '    Intercept Module (Dynamic Linker Control):',
    '    ==========================================',
    '',
    '    enabled:                                @0@'.format(with_libintercept),
    '@0@'.format('\n'.join(intercept_module_report)),
    '',
    '    Redirect Module (Standard Library Control):',
    '    ===========================================',
    '',
    '    enabled:                                @0@'.format(with_libredirect),
    '',
    '    Snap Support (run as a snap):',
    '    =============================',
    '',
    '    enabled:                                @0@'.format(with_snap_support),
    '@0@'.format('\n'.join(redirect_module_report)),
]

# Output some stuff to validate the build config
message('\n\n\n' + '\n'.join(report) + '\n\n')
