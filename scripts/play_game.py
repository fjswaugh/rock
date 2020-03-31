import rock

def ask_user_for_move(board, player):
    move = None
    while (not move):
        move = rock.Move.parse(input("Enter a move: "))
        if not move:
            print("Cannot read move")
        elif not move.is_legal(board, player):
            print("{} is an illegal move for {}".format(move, player))
            move = None
    return move

def pick_ai_move_and_evaluate(board, player):
    print("Deciding on a move...")
    ai_move, score = rock.recommend_move(board, player)
    print("{}".format(ai_move))
    return ai_move, score

def play_interactive_game():
    board = rock.starting_board
    score = 0
    i = 0
    outcome = rock.Ongoing

    while True:
        print("State after {} moves".format(i))
        print(board)
        print("Score: {}".format(score))

        human_move = ask_user_for_move(board, rock.White)
        board.apply_move(human_move)

        outcome = board.game_outcome(rock.Black)
        if outcome != rock.GameOutcome.Ongoing:
            return outcome, board

        ai_move, score = pick_ai_move_and_evaluate(board, rock.Black)
        board.apply_move(ai_move)

        outcome = board.game_outcome(rock.White)
        if outcome != rock.GameOutcome.Ongoing:
            return outcome, board

        i += 1

if __name__ == "__main__":
    outcome, board = play_interactive_game()

    print("Final board:")
    print(board)
    print("Result: {}".format(outcome))
