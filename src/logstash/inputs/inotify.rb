require "logstash/inputs/base"
require "rb-inotify"
require "find"
class LogStash::Inputs::Inotify < LogStash::Inputs::Base
    config_name "inotify"
    milestone 1
    config :directory, :validate => :string, :required => true
    public
    def register
        @offsets = {}
        @notifier = INotify::Notifier.new
        setup_watch
    end
    public
    def run output_queue
        @output_queue = output_queue
        @notifier.run
    end
    def file_changed path
        File.open(path) do |f|
            f.seek @offsets[path] ||= 0
            f.readlines.each do |line|
                @output_queue << {:path => path, :message => line}
            end
            @offsets[path] = f.tell
        end
    end
    def setup_watch
        Find.find(@directory) do |path|
            if File.directory? path
                watch_directory path if File.readable? path
            else
                @offsets[path] = File.size path
            end
        end
    end
    def watch_directory directory
        @notifier.watch(directory, :create, :modify) do |event|
            if File.directory? event.absolute_name
                watch_directory event.absolute_name
            elsif event.flags.include? :modify
                file_changed event.absolute_name 
            end
        end
    end
end
