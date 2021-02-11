from setuptools import setup
from codecs import open

setup(
	name = 'LiveKit',
	version = '0.0.1',
	description = 'A scripting interface resembling OBS Studio.',
	url='https://github.com/fynv/LiveKit',
	license='GPL v2',
	author='Fei Yang, Vulcan Eon, Beijing',
	author_email='hyangfeih@gmail.com',
	keywords='Live Streaming Video',
	packages=['LiveKit'],
	package_data = { 'LiveKit': ['*.dll']},
	install_requires = ['cffi','numpy'],
)


