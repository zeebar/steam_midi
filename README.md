# SteamMidi

this simple UDP network bridge was created to facilitate communication between 
friendly cooperating hosts on a local IP4 network, including Linux and OS/X 
systems as well as WiFi enabled Arduinos serving as MIDI instruments.

## Installation

Add this line to your application's Gemfile:

```ruby
gem 'steam_midi'
```

And then execute:

    $ bundle

Or install it yourself as:

    $ gem install steam_midi

You might need to set your jruby such:
```
rbenv local jruby-9.1.9.0
```

You will want to set your own WiFi password in arduino/esp32thing_midi/steam_private.h

It should look something like this:

```
    #define WIFI_SSID "your_wifi_name"
    #define WIFI_PASS "your_wifi_password"
```


## Usage

1. upload the arduino sketch to your midi instrument creation
2. steam_midid


## Requirements

You will need jruby installed and working.

The server host currently assumes to have the IP address 192.168.4.10.


## Notes

You might need to open your desktop/laptop firewall settings to allow steammidid to receive
UDP messages on ports 15014 and 15015.


## Contributing

Bug reports and pull requests are welcome on GitHub at https://github.com/ezeebar/steam_midi.

