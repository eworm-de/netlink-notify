project('netlink-notify', ['c'], version: '0.8.1')

src = files(
	'netlink-notify.c'
)

icons = files(
	'icons/netlink-notify-up.png',
	'icons/netlink-notify-down.png',
	'icons/netlink-notify-address.png',
	'icons/netlink-notify-away.png',
)

markdown = find_program('markdown', required: get_option('build_docs'))

libnotify_dep = dependency('libnotify')
libsystemd_dep = dependency('libsystemd', required: false)

conf_data = configuration_data()
conf_data.set('version', meson.project_version())

configure_file(input: 'config.h.in', output: 'config.h', configuration: conf_data)

c_args = []

if libsystemd_dep.found()
	c_args += ['-DHAVE_SYSTEMD']

	systemd = dependency('systemd', required: true)
	unitdir = systemd.get_pkgconfig_variable('systemduserunitdir')

	install_data('systemd/netlink-notify.service', install_dir: unitdir)
endif

docdir = join_paths(get_option('datadir'), 'doc', 'netlink-notify')
icondir = join_paths(get_option('datadir'), 'icons', 'hicolor', '48x48', 'status')

install_data(icons, install_dir: icondir)


if get_option('build_docs')
	readme_html = custom_target('readme-html',
		input: 'README.md',
		output: 'README.html',
		command: [markdown, '-o', '@OUTPUT@', '@INPUT@'],
		install: true,
		install_dir: docdir,
	)

	doc_files = files(
		'README.md',
	)

	install_data(doc_files, install_dir: docdir)
	install_subdir('screenshots', install_dir: docdir)
endif

executable('netlink-notify', src, c_args: c_args, install: true,
	dependencies: [libnotify_dep, libsystemd_dep])
