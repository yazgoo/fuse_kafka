require "base64"
require "logstash/inputs/base"
require "logstash/namespace"
require "json"
java_import "kafka.consumer.ConsumerIterator"
java_import "kafka.consumer.KafkaStream"
java_import "kafka.consumer.ConsumerConfig"
java_import "java.util.Properties"
java_import "kafka.consumer.Consumer"
java_import "java.util.HashMap"
class LogStash::Inputs::Kafka < LogStash::Inputs::Base
    config_name "kafka"
    milestone 1
    config :zkConnect, :validate => :string, :required => true
    config :groupId, :validate => :string, :required => true
    config :topic, :validate => :string, :required => true
    config :num_threads, :validate => :string, :required => true
    public
    def register
        begin
            @logger.info "registering kafka logger"
            properties = Properties.new
            properties.put "zookeeper.connect", @zkConnect
            properties.put "group.id", @groupId
            consumer = Consumer.createJavaConsumerConnector(
                ConsumerConfig.new properties)
            @logger.info "done creating consumer connector"
            topic_count_map = HashMap.new
            topic_count_map.put @topic, @num_threads.to_i.to_java(:int)
            @streams = consumer.createMessageStreams(
                topic_count_map).get @topic
                @logger.info "done creating message streams"
        rescue => e
            puts e
            begin
                e.printStackTrace if e.printStackTrace
            rescue
            end
            raise e
        end
    end
    public
    def run queue
        threads = []
        @streams.each do |stream|
            threads << Thread.new do
                stream.each do |packet|
                    begin
                        json = String.from_java_bytes(packet.message).gsub("\n", "\n").gsub("\t", "\t")
                        hash = ::JSON.parse(json)
                        {"command" => [">=0.1.2"], "@message" => [">=0.1.2", "0.2"]}.each do |f, vs|
                            if vs.collect { |x| Gem::Dependency.new("", x).match? "", hash["@version"] }.reduce { |x, y| x | y }
                                hash[f] = ::Base64.decode64(hash[f])
                            end
                            hash[f] = hash[f].encode("UTF-8", :invalid => :replace, :undef => :replace)
                        end
                        queue << LogStash::Event.new(hash)
                    rescue => e
                        puts e
                    end
                end
            end
        end
        threads.each { |thread| thread.join }
    end
end
