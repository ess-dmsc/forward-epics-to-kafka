require './src/librbext01'
require 'json'

class Main

	def start
		@threads = []
		return
		2.times do |i1|
			@threads << Thread.new do |x|
				40.times do |i2|
					p [i1, i2]
					RBEXT01.new.update
					sleep 0.1
				end
			end
		end
	end

	def watch
		begin
			p 'go go go'
			watch_protected
		rescue LoadError => e
			puts 'rescuing'
			p e
		rescue => e
			puts "Unhandled exception: #{e}"
		end
	end

	def watch_protected
		require 'irb'
		IRB.start
		puts "AFTER IRB!"
		return
		require 'socket'
		sock_in = UDPSocket.new
		sock_in.bind 'localhost', 3131
		while true
			buf1 = sock_in.gets "\0"
			puts "Received block: #{buf1}"
		end
	end

	def tried_to_run_rails
		# But the middleware servers all run in their own processes...
		Dir.chdir "#{ENV['HOME']}/rails-app-01"
		ARGV.clear
		ARGV << 'server'
		p ARGV
		load '/home/scratch/software/ruby-2.3.1/inst/bin/rails'
	end

	def join
		@threads.each do |x| x.join end
		puts 'main exit'
	end

end

if @m
	puts '@m is already alive'
else
	@m = Main.new
	@m.start
end
