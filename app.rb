require 'sinatra/base'
require 'sinatra/flash'
require 'json'

class RemoteExoApp < Sinatra::Base
  enable :sessions
  register Sinatra::Flash

  # Store the shared value in memory (server state)
  SHARED_STATE = {}

  # Route to display the user input form
  get '/' do
    erb :index
  end

  # Route to set the shared value
  post '/set_value' do
    key = params[:key] || "default_key" # Key for the value (optional)
    value = params[:value]             # Value to store
    SHARED_STATE[key] = value          # Store the value in the shared state

    flash[:notice] = "Value set successfully: #{key} = #{value}"
    redirect '/'
  end

  # Route for Arduino to retrieve the value
  get '/get_value' do
    key = params[:key] || "default_key" # Key to look up (optional)
    value = SHARED_STATE[key]           # Retrieve the value from shared state

    if value
      content_type :json
      { key: key, value: value }.to_json # Return the value as JSON
    else
      halt 404, { error: "Key not found: #{key}" }.to_json
    end
  end
end
