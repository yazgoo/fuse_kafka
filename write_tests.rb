require 'poseidon'
require 'json'
require 'csv'
require 'base64'
require 'awesome_print'
class Tester
    attr_reader :events
    def initialize
        start = Time.new
        dir = "/tmp/fuse-kafka-test/"
        started = false
        @events = []
        n_partition = 2
        stop_file = dir + "stop"
        start_file = dir + "start"
        write_file = dir + "blah"
        write_files = []
        done = false
        threads = n_partition.times.collect do |partition|
            Thread.new do
                consumer = Poseidon::PartitionConsumer.new("_", "localhost", 9092, "logs", partition, :latest_offset)
                loop do
                    break if done
                    consumer.fetch.each do |m|
                        event = JSON.parse(m.value)
                        if event["path"] == start_file
                            started = true
                            break
                        end
                        done = (event["path"] == stop_file)
                        ["@message", "command"].each do |w|
                            event[w] = Base64.decode64(event[w])
                        end
                        break if done
                        @events.push event
                    end
                    break if done
                end
            end
        end
        @n = 0
        loop { sleep 1; File.write(start_file, "startup"); break if started }
        ["a", "w", "w+", "a", "a+"].each do |mode|
            [true, false].each do |flush|
                write_files += [write_file + @n.to_s]
                File.open(write_files.last, mode) do |f|
                    100.times do
                        f.write "#{@n} "
                        @n += 1
                        f.flush if flush
                    end
                end
            end
        end
        (n_partition * 10).times.each { File.write(stop_file, "shutdown") }
        threads.each { |t| t.join }
        write_files + [stop_file, start_file].each { |f| File.delete(f) }
        @duration = Time.new - start
    end
    def save path
        values = []
        File.open(path, "w") do |f|
            f.puts "duration: #{@duration}"
            @events.each do |event|
                values += event["@message"].split(" ").collect { |x| x.to_i }
                f.puts "#{event["@timestamp"]} #{event["path"]}: #{event["@message"]}"
            end
        end
        print_missing values
    end
    def print_missing_summary first, last
        if not first.nil?
            puts "#{first}...#{last} missing"
        end
        nil
    end
    def print_missing values
        first = nil
        last = nil
        @n.times do |i|
            if not values.include?(i)
                if not first.nil?
                    last = i
                else
                    last = first = i
                end
            else
                first = print_missing_summary first, last
            end
        end 
        print_missing_summary first, last
        puts "saved #{@events.size} events"
    end
end
tester = Tester.new
tester.save ARGV[0]
