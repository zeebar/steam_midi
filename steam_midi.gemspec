# coding: utf-8
lib = File.expand_path('../lib', __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)
require 'steam_midi/version'

Gem::Specification.new do |spec|

  spec.name          = "steam_midi"
  spec.version       = SteamMidi::VERSION
  spec.authors       = ["Helmut Hissen"]
  spec.email         = ["helmut@zeebar.com"]

  spec.platform      = 'java'

  spec.summary       = %q{simple scriptable MIDI/Arduino sound bridge}
  spec.description   = %q{a simple and scriptable cross-platform MIDI/Arduino sound bridge}
  spec.homepage      = "http://www.zeebar.com/gems/steam_midi"

  # Prevent pushing this gem to RubyGems.org. To allow pushes either set the 'allowed_push_host'
  # to allow pushing to a single host or delete this section to allow pushing to any host.
  if spec.respond_to?(:metadata)
  else
    raise "RubyGems 2.0 or newer is required to protect against " \
      "public gem pushes."
  end

  spec.files         = `git ls-files -z`.split("\x0").reject do |f|
    f.match(%r{^(test|spec|features)/})
  end
  spec.bindir        = "exe"
  spec.executables   = spec.files.grep(%r{^exe/}) { |f| File.basename(f) }
  spec.require_paths = ["lib"]

  spec.add_dependency "midi-eye"

  spec.add_development_dependency "bundler", "~> 1.14"
  spec.add_development_dependency "rake", "~> 10.0"
  spec.add_development_dependency "rspec", "~> 3.0"
end
