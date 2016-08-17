import sys
#import subprocess
import os

class Foo:
  def add(self):
    for i1 in range(0, int(sys.argv[2])):
      topic_id = i1
      if topic_id > 24:
        topic_id = 24
      cmd = str.format("./config-msg --broker-configuration-address localhost --add --channel pv.%06d --topic pv.%06d" % (i1, topic_id))
      print(cmd)
      os.system(cmd)

  def remove(self):
    for i1 in range(0, int(sys.argv[2])):
      topic_id = i1
      if topic_id > 24:
        topic_id = 24
      cmd = str.format("./config-msg --broker-configuration-address localhost --remove --channel pv.%06d" % (i1))
      print(cmd)
      os.system(cmd)


x = Foo()
m = getattr(x, sys.argv[1])
m()
