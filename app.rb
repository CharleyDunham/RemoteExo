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
    value = params[:value]              # Value to store
    SHARED_STATE[key] = value           # Store the value in the shared state

    flash[:notice] = "Value set successfully: #{key} = #{value}"
    redirect '/'
  end

  # Route to get a specific value by key
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

  # Route to set the flexion state
  post '/set_flexion' do
    flexion_state = params[:flexion] || "0" # Default flexion value
    SHARED_STATE["flexion"] = flexion_state # Store the flexion state in shared state

    flash[:notice] = "Flexion state set to: #{flexion_state}"
    redirect '/'
  end

  # Route to get the flexion state
  get '/get_flexion' do
    value = SHARED_STATE["flexion"] # Retrieve the flexion value from shared state

    if value
      content_type :text
      value # Return the flexion value as plain text
    else
      halt 404, "Flexion value not set" # Return a 404 error with a plain text message
    end
  end
end
