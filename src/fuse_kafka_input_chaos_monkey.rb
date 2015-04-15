#!/usr/bin/env ruby
require 'matrix'
require 'fileutils'
def get_last_segfault
    last = `dmesg|tail`.split("\n").select { |x| x =~ /.*segfault.*/ }[-1]
    if last.nil? then 0 else last.split[1].to_i end
end
def each_directories selected
    selected.each do |x|
        x[1].each { |d| yield d if not d.empty? } if x[0] == "directories"
    end
end
@last_segfault = get_last_segfault
def start selected
    gdb = "gdb -ex 'set follow-fork-mode child' -ex run -ex quit --args "
    prefix = "./fuse_kafka _ -oallow_other -ononempty -omodules=subdir,subdir=. -f -- "
    each_directories(selected) { |d| FileUtils.mkdir_p d }
    cmd =  selected.collect do |x|
        x[1].each { |d| FileUtils.mkdir_p d if not d.empty? } if x[0] == "directories"
        "--#{([x[0]]+x[1]).join " "}" 
    end
    segfault = get_last_segfault
    if segfault > @last_segfault
        puts "segfaulted #{segfault}"
        exit(1)
    end
    puts "cmd: '#{cmd.join(" ")}'"
    if ARGV.size == 0
        exit(1) if not Process.fork { system(prefix + cmd.join(" ")) }
    else # reload mode, you should also change sleep time in dynamic_configuration.c
        File.delete("/tmp/done") if File.exist?("/tmp/done")
        File.open("/var/run/fuse_kafka.args", "w") do |f|
            f.flock(File::LOCK_EX)
            f.write(cmd.join(" "))
        end
        loop do
            break if File.exist?("/tmp/done")
        end
        sleep(0.01)
    end
    each_directories(selected) { |d| sleep(2);umount = "fusermount -u #{d}"; puts umount; system(umount) }
end
dir = File.dirname(__FILE__)
options = File.read(dir + "/fuse_kafka.c").each_line.collect do |line|
    match = line.match /^ *CONFIG_ITEM\((.*)\)$/
    match[1] if not match.nil?
end.select { |x| x }
count = 4 # options.size
count.times do |i|
    puts "=======#{i}=============="
    params = [[], [""], ["blah"], ["blah", "foo"], ["pif"], ["paf"], ["pouf"], []][0..i].permutation(i).to_a
    options.combination(i).to_a.each do |combination|
        params.each do |param|
            start combination.zip param
        end
    end
end
