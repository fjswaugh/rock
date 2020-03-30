import rock

def ask_user_for_move(board, player_color):
    move = None
    while (not move):
        move = rock.Move.parse(input("Enter a move: "))
        if not move:
            print("Cannot read move")
        elif not rock.is_valid_move(move, board, player_color):
            print("{} is an illegal move for {}".format(move, player_color))
            move = None
    return move

def pick_ai_move(board, ai_player_color):
    print("Deciding on a move...")
    ai_move = rock.recommend_move(board, rock.Black)
    print("{}".format(ai_move))
    return ai_move

def play_interactive_game():
    board = rock.make_starting_board()
    i = 0

    while not board.winning_player():
        print("State after {} moves".format(i))
        print(board)

        human_move = ask_user_for_move(board, rock.White)
        board.apply_move(human_move)
        ai_move = pick_ai_move(board, rock.Black)
        board.apply_move(ai_move)
        i += 1

    print("Final board:")
    print(board)
    print("Winner: {}".format(board.winning_player()))

if __name__ == "__main__":
    play_interactive_game()
