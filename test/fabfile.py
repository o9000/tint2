#!/usr/bin/env python2

# Setup: add tint2-runner, tint2-freebsd and tint2-openbsd in /etc/hosts.
# Run: pip install fabric; pip install fabtools.

# TODO: setup bsd workers
# TODO: prin ssh public key to be added on gitlab

from fabric.api import *
from fabric.contrib.files import *
from fabtools import require
import fabtools
import os


env.use_ssh_config = True
env.user = 'root'
env.sudo_prefix += '-H '
env.roledefs = {
    'runner': ['tint2-runner'],
    'freebsd': ['tint2-freebsd'],
    'openbsd': ['tint2-openbsd'],
}


def str2hex(s):
  return ''.join('{:02x}'.format(ord(c)) for c in s)


def generate_random_password():
  return str2hex(os.urandom(32))


def read_file(path):
  with open(path) as f:
    return f.read()


@task
@roles('runner', 'freebsd', 'openbsd')
def create_users():
  require.user('root', password=generate_random_password())
  require.user('runner', password=generate_random_password())
  sudo('cd; mkdir -p .ssh; chmod 700 .ssh', user='runner')
  if not exists('/home/runner/.ssh/id_rsa'):
    sudo('cd; ssh-keygen -f ~/.ssh/id_rsa -t rsa -N ""', user='runner')


@task
@roles('runner')
def install_deps():
  require.deb.packages([
    # Repo deps
    'git',
    # Build deps
    'build-essential',
    'cmake',
    'libglib2.0-dev',
    'libcairo2-dev',
    'libglib2.0-dev',
    'libgtk2.0-dev',
    'libimlib2-dev',
    'libpango1.0-dev',
    'librsvg2-dev',
    'libstartup-notification0-dev',
    'libx11-dev',
    'libxcomposite-dev',
    'libxdamage-dev',
    'libxinerama-dev',
    'libxrandr-dev',
    'libxrender-dev',
    # Tester deps
    'python-minimal',
    'xvfb',
    'xsettingsd',
    'openbox',
    'compton',
    'x11-utils',
    'gnome-calculator'
  ])


@task
@roles('runner')
def pull_code():
  if not exists('/home/runner/tint2'):
    sudo('cd; git clone https://gitlab.com/o9000/tint2.git', user='runner')
  if not exists('/home/runner/tint2.wiki'):
    sudo('cd; git clone git@gitlab.com:o9000/tint2.wiki.git', user='runner')


@task
@roles('runner')
def add_cron_jobs():
  fabtools.cron.add_task('tests', '* * * * *', 'runner', '/home/runner/tint2/test/update_test_status.sh')
  fabtools.cron.add_task('packaging_check', '10 */2 * * *', 'runner', '/home/runner/tint2/packaging/update_version_status.sh')


@task
@roles('runner')
def full_runner():
  create_users()
  install_deps()
  pull_code()
  add_cron_jobs()
