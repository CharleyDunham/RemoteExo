require 'sinatra/base'
require 'sinatra/flash'
require 'sinatra'
require 'json'
require_relative './lib/wordguesser_game.rb'
require_relative './lib/arduino_communicator.rb'


class WordGuesserApp < Sinatra::Base

  enable :sessions
  register Sinatra::Flash

  before do
    @game = session[:game] || WordGuesserGame.new('')
  end

  after do
    session[:game] = @game
  end

  # These two routes are good examples of Sinatra syntax
  # to help you with the rest of the assignment
  get '/' do
    redirect '/new'
  end

  get '/new' do
    erb :new
  end

  post '/create' do
    # NOTE: don't change next line - it's needed by autograder!
    word = params[:word] || WordGuesserGame.get_random_word
    # NOTE: don't change previous line - it's needed by autograder!

    @game = WordGuesserGame.new(word)
    redirect '/show'
  end

  # Use existing methods in WordGuesserGame to process a guess.
  # If a guess is repeated, set flash[:message] to "You have already used that letter."
  # If a guess is invalid, set flash[:message] to "Invalid guess."
  post '/guess' do
    letter = params[:guess].to_s[0]
    ### YOUR CODE HERE ###
    if letter.nil? || letter.empty? || !letter.match?(/[a-zA-Z]/)
      flash[:message] = 'Invalid guess.'
    elsif @game.guesses.include?(letter) || @game.wrong_guesses.include?(letter)
      flash[:message] = 'You have already used that letter.'
    else
      @game.guess(letter)
    end

    redirect '/show'
  end

  # Everytime a guess is made, we should eventually end up at this route.
  # Use existing methods in WordGuesserGame to check if player has
  # won, lost, or neither, and take the appropriate action.
  # Notice that the show.erb template expects to use the instance variables
  # wrong_guesses and word_with_guesses from @game.
  get '/show' do
    ### YOUR CODE HERE ###
    case @game.check_win_or_lose
    when :win
      redirect '/win'
    when :lose
      redirect '/lose'
    end

    erb :show # You may change/remove this line
  end

  get '/win' do
    ### YOUR CODE HERE ###
    if @game.check_win_or_lose == :win
      erb :win # You may change/remove this line
    else
      redirect '/show'
    end
  end

  get '/lose' do
    ### YOUR CODE HERE ###
    if @game.check_win_or_lose == :lose
      erb :lose # You may change/remove this line
    else
      redirect '/show'
    end
  end


  post '/send_arduino_command' do
    communicator = ArduinoCommunicator.new
    command = params[:command] # Assume this comes from a form or input

    result = communicator.send_command(command)
    if result
      flash[:message] = "Arduino command sent successfully: #{result}"
    else
      flash[:message] = "Failed to send command to Arduino."
    end

    redirect '/show'
  end

  post '/receive-data' do
    request.body.rewind  # In case someone already read it
    payload = JSON.parse(request.body.read) rescue {}
    puts "Received data from Arduino: #{payload}"

    # Respond to the Arduino (can be customized)
    content_type :json
    { status: 'success', message: 'Data received' }.to_json
  end

  get '/retrieve-data' do
    content_type :json
    { message: 'Data retrieved successfully', data: 'Some example data' }.to_json
  end

end

