class WordGuesserGame

  # add the necessary class methods, attributes, etc. here
  # to make the tests in spec/wordguesser_game_spec.rb pass.
  attr_accessor :word, :guesses, :wrong_guesses

  # Get a word from remote "random word" service

  def initialize(word)
    @word = word
    @guesses = ''
    @wrong_guesses = ''
  end

  def guess(letter)
    raise ArgumentError if letter.nil? || letter.empty? || !letter.match?(/[a-zA-Z]/)

    letter.downcase!
    @word.downcase

    # checks for letters that are already guessed
    return false if @guesses.include?(letter) || @wrong_guesses.include?(letter)

    # adds the letter to the guesses if it is in the word
    @guesses += letter if @word.include?(letter)
    # adds to wrong guesses if it is not in the word
    @wrong_guesses += letter unless @word.include?(letter)

    true
  end

  def word_with_guesses
    displayed_word = ''
    @word.chars.each do |char|
      if @guesses.downcase.include?(char.downcase)
        displayed_word << char
      else
        displayed_word << '-'
      end
    end
    displayed_word
  end

  def check_win_or_lose
    if (@word.chars - @guesses.chars).empty?
      return :win
    end

    if @wrong_guesses.length >= 7
      return :lose
    end

    :play
  end

  # You can test it by installing irb via $ gem install irb
  # and then running $ irb -I. -r app.rb
  # And then in the irb: irb(main):001:0> WordGuesserGame.get_random_word
  #  => "cooking"   <-- some random word
  def self.get_random_word
    require 'uri'
    require 'net/http'
    uri = URI('http://randomword.saasbook.info/RandomWord')
    Net::HTTP.new('randomword.saasbook.info').start { |http|
      return http.post(uri, '').body
    }
  end

end
