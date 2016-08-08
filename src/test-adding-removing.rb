def run cmd
	puts cmd
	system cmd
end

while true
	1.times do |i1|
		i1 = 50 * Kernel.rand
		cmd = sprintf "./config-msg --add --channel pv.%06d --topic pv.%06d", i1, i1 < 8 ? i1 : 8
		run cmd
	end
	sleep 0.01
	1.times do |i1|
		i1 = 50 * Kernel.rand
		cmd = sprintf "./config-msg --remove --channel pv.%06d", i1
		run cmd
	end
	sleep 0.01
end
