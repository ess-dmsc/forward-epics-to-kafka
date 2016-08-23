import sys
#import subprocess
import os
import argparse

class Main:
  def __init__(self):
    self.host = "ess01"
    self.config_msg = "/opt/local/manual-forwarder/build/config-msg";

    parser = argparse.ArgumentParser(description='Test forwarder with many processes.')
    #parser.add_argument('--add', action='store_true')
    parser.add_argument('--add', type=int)
    parser.add_argument('--addR', nargs=2, type=int)
    parser.add_argument('--remove')
    parser.add_argument('--removeR')
    parser.add_argument('--batch', nargs=2, type=int)
    parser.add_argument('--broker-configuration-address', dest='broker-configuration-address', default='localhost')
    parser.add_argument('--broker-configuration-topic', dest='broker-configuration-topic', default='configuration.global')
    self.args = parser.parse_args()
    if self.args.add:
      self.args.addR = [0, self.args.add - 1]
    if self.args.addR:
      self.add()
    if self.args.remove:
      self.remove()
    if self.args.batch:
      self.batch(self.args.batch[0], self.args.batch[1])

  def add(self):
    r = self.args.addR
    for i1 in range(r[0], 1 + r[1]):
      topic_id = min(1000, i1)
      cmd = str.format(self.config_msg + " --broker-configuration-address %s --broker-configuration-topic %s --add --channel pv.%06d --topic pv.%06d"
        % (getattr(self.args, 'broker-configuration-address'), getattr(self.args, 'broker-configuration-topic'), i1, topic_id))
      print(cmd)
      os.system(cmd)

  def remove(self):
    for i1 in range(0, int(self.args.remove)):
      cmd = str.format(self.config_msg + " --broker-configuration-address %s --broker-configuration-topic %s --add --channel pv.%06d"
        % (getattr(self.args, 'broker-configuration-address'), getattr(self.args, 'broker-configuration-topic'), i1))
      print(cmd)
      os.system(cmd)

  def batch(self, n_forwarders, n_topics):
    for i1 in range(0, n_forwarders):
      t1 = n_topics * i1
      t2 = n_topics * i1 + (n_topics - 1)
      os.system(str.format('python ~/dev/ESS-ICS/forward-epics-to-kafka/repo/src/test-add-many.py --broker-configuration-address=ess01 --broker-configuration-topic=conf%d --addR  %d  %d' % (i1, t1, t2)))


x = Main()
