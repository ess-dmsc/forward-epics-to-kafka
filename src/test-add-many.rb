def run cmd
	puts cmd
	system cmd
end

def add
	ARGV[1].to_i.times do |i1|
		topic_id = [i1, 24].min
		cmd = sprintf "./config-msg --broker-configuration-address ess01 --add --channel pv.%06d --topic pv.%06d", i1, topic_id
		run cmd
	end
end

def remove
	32.times do |i1|
		cmd = sprintf "./config-msg --broker-configuration-address ess01 --remove --channel pv.%06d", i1
		run cmd
	end
end

send ARGV[0].to_sym
