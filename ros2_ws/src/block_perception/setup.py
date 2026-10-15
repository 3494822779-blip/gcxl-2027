from glob import glob
from setuptools import setup
setup(name='block_perception', version='0.1.0', packages=['block_perception'],
      data_files=[('share/ament_index/resource_index/packages', ['resource/block_perception']),
                  ('share/block_perception', ['package.xml']),
                  ('share/block_perception/launch', glob('launch/*.py')),
                  ('share/block_perception/config', glob('config/*'))],
      install_requires=['setuptools', 'numpy'], zip_safe=True,
      entry_points={'console_scripts': [f'{n} = block_perception.nodes:{n}_main' for n in ['camera', 'selector', 'filter', 'alignment', 'annotations']]})
