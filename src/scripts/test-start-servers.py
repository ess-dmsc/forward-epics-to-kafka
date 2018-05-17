import os
import argparse

class StartServers:
  def __init__(self):
    parser = argparse.ArgumentParser(description='Start/stop forwarders for performance testing.')
    # action='store_true'
    parser.add_argument('--start', type=int)
    parser.add_argument('--stop', action='store_true')
    self.args = parser.parse_args()
    if self.args.start:
      self.start(self.args.start)
    if self.args.stop:
      self.stop()

  def start(self, n1):
    for i1 in range(0, n1):
      os.system(str.format('tcsh ~/s1 %d > log_%d &' % (i1, i1)))

  def stop(self):
    os.system('killall forward-epics-to-kafka')

StartServers()
