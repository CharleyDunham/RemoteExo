# lib/arduino_communicator.rb
require 'httparty'

class ArduinoCommunicator
  include HTTParty
  base_uri 'http://arduino-device.local' # Replace with your actual Arduino URL

  def initialize
    @headers = { "Content-Type" => "application/json" }
  end

  def send_command(command)
    response = self.class.get("/command", query: { value: command }, headers: @headers)
    response.success? ? response.parsed_response : nil
  rescue HTTParty::Error => e
    "Error: #{e.message}"
  end
end
