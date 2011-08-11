#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2010 Enrico Tröger <enrico(dot)troeger(at)uvena(dot)de>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

#
# Modified by Colomban Wendling <ban(at)herbesfolles(dot)org> 2010
#

"""
This is a WAF build script (http://code.google.com/p/waf/).
Requires WAF 1.5.7 and Python 2.4 (or later).
"""


import Build
import Options
import Utils
import sys
import os
import tempfile


APPNAME		= 'ctpl'
VERSION		= '0.3.3'
LTVERSION	= '2.2.1' # emulate Libtool version

srcdir = '.'
blddir = '_build_'


HEADERS = [
'src/ctpl.h',
'src/ctpl-environ.h',
'src/ctpl-eval.h',
'src/ctpl-io.h',
'src/ctpl-input-stream.h',
'src/ctpl-lexer.h',
'src/ctpl-lexer-expr.h',
'src/ctpl-output-stream.h',
'src/ctpl-parser.h',
'src/ctpl-token.h',
'src/ctpl-value.h',
'src/ctpl-version.h']

LIBRARY_SOURCES = '''
src/ctpl-environ.c
src/ctpl-eval.c
src/ctpl-io.c
src/ctpl-input-stream.c
src/ctpl-lexer.c
src/ctpl-lexer-expr.c
src/ctpl-mathutils.c
src/ctpl-output-stream.c
src/ctpl-parser.c
src/ctpl-stack.c
src/ctpl-token.c
src/ctpl-value.c
src/ctpl-version.c'''


CTPL_SOURCES = ['src/ctpl.c']


def configure(conf):

	def conf_define_from_opt(define_name, opt_name, default_value, quote=1):
		if opt_name:
			if isinstance(opt_name, bool):
				opt_name = 1
			conf.define(define_name, opt_name, quote)
		elif default_value:
			conf.define(define_name, default_value, quote)

	conf.check_tool('compiler_cc')
	is_win32 = target_is_win32(conf.env)

	# check for cxx after the header and function checks have been done to ensure they are
	# checked with cc not cxx
	conf.check_tool('misc')

	# GTK / GIO version check
	conf.check_cfg(package='glib-2.0', atleast_version='2.10.0', uselib_store='GLIB',
		mandatory=True, args='--cflags --libs')
	conf.check_cfg(package='gio-2.0', uselib_store='GIO', args='--cflags --libs', mandatory=True)
	conf.check_cfg(package='gio-2.0', atleast_version='2.24.0', uselib_store='GIO_2_24', args='--cflags --libs', mandatory=False)
	conf.check_cfg(package='gio-unix-2.0', uselib_store='GIO_UNIX', args='--cflags --libs', mandatory=False)

	# Windows specials
	if is_win32:
		if conf.env['PREFIX'] == tempfile.gettempdir():
			# overwrite default prefix on Windows (tempfile.gettempdir() is the Waf default)
			conf.define('PREFIX', os.path.join(conf.srcdir, '%s-%s' % (APPNAME, VERSION)), 1)
		conf.define('DOCDIR', os.path.join(conf.env['PREFIX'], 'doc'), 1)
		conf.define('BINDIR', os.path.join(conf.env['PREFIX'], 'bin'), 1)
		conf.define('LIBDIR', os.path.join(conf.env['PREFIX'], 'lib'), 1)
		# DATADIR is defined in objidl.h, so we remove it from config.h but keep it in env
		conf.undefine('DATADIR')
		conf.env['DATADIR'] = os.path.join(conf.env['PREFIX'], 'data')
		# we don't need -fPIC when compiling on or for Windows
		if '-fPIC' in conf.env['shlib_CCFLAGS']:
			conf.env['shlib_CCFLAGS'].remove('-fPIC')
		conf.env.append_value('program_LINKFLAGS', '-mwindows')
	else:
		conf.env['DATADIR'] = os.path.join(conf.env['PREFIX'], 'share')
		conf_define_from_opt('DOCDIR', Options.options.docdir,
			os.path.join(conf.env['DATADIR'], 'doc'))
		conf_define_from_opt('LIBDIR', Options.options.libdir,
			os.path.join(conf.env['PREFIX'], 'lib'))
		conf_define_from_opt('MANDIR', Options.options.mandir,
			os.path.join(conf.env['DATADIR'], 'man'))

	conf.define('CTPL_DATADIR', 'data' if is_win32 else conf.env['DATADIR'], 1)
	conf.define('CTPL_DOCDIR', conf.env['DOCDIR'], 1)
	conf.define('CTPL_LIBDIR', '' if is_win32 else conf.env['LIBDIR'], 1)
	conf.define('CTPL_PREFIX', '' if is_win32 else conf.env['PREFIX'], 1)
	conf.define('PACKAGE', APPNAME, 1)
	conf.define('VERSION', VERSION, 1)

	conf.write_config_header('config.h')

	Utils.pprint('BLUE', 'Summary:')
	print_message(conf, 'Install CTPL ' + VERSION + ' in', conf.env['PREFIX'])

	# FIXME: this one should not be defined for the ctpl program
	conf.env.append_value('CCFLAGS', '-DCTPL_COMPILATION')

	conf.env.append_value('CCFLAGS', '-DHAVE_CONFIG_H')
	conf.env.append_value('CCFLAGS', '-Wall')
	#~ conf.env.append_value('CCFLAGS', '-Werror')


def set_options(opt):
	opt.tool_options('compiler_cc')

	if 'configure' in sys.argv:
		# Paths
		opt.add_option('--mandir', type='string', default='',
			help='man documentation', dest='mandir')
		opt.add_option('--docdir', type='string', default='',
			help='documentation root', dest='docdir')
		opt.add_option('--libdir', type='string', default='',
			help='object code libraries', dest='libdir')


def build(bld):
	is_win32 = target_is_win32(bld.env)

	# libctpl.so, ctpl.dll
	bld.new_task_gen(
		features				= 'cc cshlib',
		source					= LIBRARY_SOURCES,
		includes				= '. src',
		name					= 'ctpl_lib',
		target					= 'ctpl',
		vnum					= LTVERSION,
		uselib					= 'GLIB GIO',
		export_incdirs			= '.'
	)

	# CTPL
	# ctpl.c doesn't build on Windows currently because of g_unix_output_stream_new()
	if not is_win32:
		bld.new_task_gen(
			features		= 'cc cprogram',
			name			= 'ctpl_utility',
			target			= 'ctpl',
			source			= CTPL_SOURCES,
			includes		= '. src',
			uselib			= 'GLIB GIO_2_24 GIO_UNIX',
			uselib_local	= 'ctpl_lib'
		)

		# ctpl.1
		bld.install_files('${MANDIR}/man1', 'data/ctpl.1')

	# ctpl.pc
	bld.new_task_gen(
		features		= 'subst',
		source			= 'data/ctpl.pc.in',
		target			= 'ctpl.pc',
		dict			= { 'VERSION' : VERSION,
							'prefix': bld.env['PREFIX']},
		install_path	= '${LIBDIR}/pkgconfig'
	)

	###
	# Install files
	###
	# Headers
	bld.install_files('${PREFIX}/include/ctpl', HEADERS)
	# Docs
	base_dir = '${PREFIX}' if is_win32 else '${DOCDIR}/ctpl'
	ext = '.txt' if is_win32 else ''
	html_dir = '' if is_win32 else 'html/'
	for f in ('AUTHORS', 'COPYING', 'ChangeLog', 'README', 'NEWS', 'TODO', 'THANKS'):
		if os.path.exists(f):
			bld.install_as("%s/%s%s" % (base_dir, uc_first(f, is_win32), ext), f)
	html_docsrcdir = 'docs/reference/ctpl/html'
	if not os.path.isdir(html_docsrcdir):
		print("** Documentation not found, it will not be installed.")
	else:
		for f in os.listdir(html_docsrcdir):
			bld.install_files('${DOCDIR}/../gtk-doc/html/ctpl/', os.path.join(html_docsrcdir, f))
		# add a symlink in the docdir
		bld.symlink_as('${DOCDIR}/ctpl/html', '../../gtk-doc/html/ctpl/')


def print_message(conf, msg, result, color = 'GREEN'):
	conf.check_message_1(msg)
	conf.check_message_2(result, color)


def uc_first(s, is_win32):
	if is_win32:
		return s.title()
	return s


def target_is_win32(env):
	if sys.platform == 'win32':
		return True
	if env and 'CC' in env:
		cc = env['CC']
		if not isinstance(cc, str):
			cc = ''.join(cc)
		return cc.find('mingw') != -1
	return False
