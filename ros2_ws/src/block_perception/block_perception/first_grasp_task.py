"""Compatibility executable: same explicit-class selector as the normal launch."""
from .nodes import Selector, run


class FirstGraspTask(Selector):
    def __init__(self):
        super().__init__('first_grasp')


def main():
    run(FirstGraspTask)
