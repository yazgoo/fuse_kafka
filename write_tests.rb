require 'poseidon'
require 'json'
require 'csv'
require 'base64'
require 'awesome_print'
class Tester
    attr_reader :events
    def initialize
        dir = "/tmp/fuse-kafka-test/"
        started = false
        @events = []
        n_partition = 2
        stop_file = dir + "stop"
        start_file = dir + "start"
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
        n = 0
        loop { sleep 1; File.write(start_file, "startup"); break if started }
        ["a", "w", "w+", "a", "a+"].each do |mode|
            [true, false].each do |flush|
                File.open(dir + "blah", mode) do |f|
                    100.times do
                        f.write "#{n} "
                        n += 1
                        f.flush if flush
                    end
                end
            end
        end
        n_partition.times.each { File.write(stop_file, "shutdown") }
        threads.each { |t| t.join }
    end
end
tester = Tester.new
File.open(ARGV[0], "w") do |f|
    tester.events.each do |event|
        f.puts "#{event["@timestamp"]} #{event["@path"]}: #{event["@message"]}"
    end
end
