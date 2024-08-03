#include <map>
#include <list>
#include <algorithm>
#include <vector>

#if WITH_EDITOR
#include <CoreMinimal.h>
DEFINE_LOG_CATEGORY_STATIC(LogChessEngine, Log, All);
#endif

using namespace std;

struct Position {

    Position() {}
    Position(int32 x, int32 y): x(x), y(y) {}

    int32 x, y;
};

class Cell {
public:
    enum PieceType {
        none, pawn, knight, bishop, rook, queen, king
    };

    enum PieceColor {
        absent, white, black
    };

    Cell() {}
    Cell(PieceType pt, PieceColor pc): piece(pt), piece_color(pc) {}
    Cell(Cell& other): piece(other.piece), piece_color(other.piece_color) {}

    void set_piece(PieceType pt, PieceColor pc) {
        piece = pt;
        piece_color = pc;
    }

    void remove_piece() {
        piece = PieceType::none;
        piece_color = PieceColor::absent;
    }

    bool has_piece() {
        return piece != PieceType::none;
    }

    bool has_white_piece() {
        return piece != PieceType::none && piece_color == PieceColor::white;
    }

    bool has_black_piece() {
        return piece != PieceType::none && piece_color == PieceColor::black;
    }

    bool has_piece_of_same_color(Cell* cell) {
        PieceColor other_color = cell->get_piece_color();
        return piece != PieceType::none
               && piece_color != PieceColor::absent
               && other_color != PieceColor::absent
               && piece_color == other_color;
    }

    bool has_piece_of_opposite_color(Cell* cell) {
        PieceColor other_color = cell->get_piece_color();
        return piece != PieceType::none
               && piece_color != PieceColor::absent
               && other_color != PieceColor::absent
               && piece_color != other_color;
    }

    PieceType get_piece_type() {
        return piece;
    }

    PieceColor get_piece_color() {
        return piece_color;
    }
    
    PieceColor get_opposite_color() {
        switch (piece_color) {
            case PieceColor::white:
                return PieceColor::black;
            case PieceColor::black:
                return PieceColor::white;
        }
        return PieceColor::absent;
    }
    
private:
    PieceType piece = PieceType::none;
    PieceColor piece_color = PieceColor::absent;
};

/**
 * @class Board
 * @brief Represents the chess board and its operations.
 * 
 * The Board class is responsible for managing the chess board and performing various operations on it,
 * such as moving pieces, evaluating the board state, and generating valid moves.
 * It also stores the piece values used for scoring and evaluation.
 */
class Board {
public:

    map<Cell::PieceType, int32> piece_values = {
        {Cell::PieceType::pawn, 1},
        {Cell::PieceType::knight, 3},
        {Cell::PieceType::bishop, 3},
        {Cell::PieceType::rook, 5},
        {Cell::PieceType::queen, 9},
        {Cell::PieceType::king, 100}
    };

    /**
     * @brief Constructor for the Board class.
     * 
     * Initializes the chess board and creates the cells for each position.
     */
    Board() {
        for (int32 x = 0; x <= max; x++) {
            int32 y_max = median + x;
            if (y_max > max) {
                y_max = max - y_max % max;
            }
            for (int32 y = 0; y <= y_max; y++) {
                int32 pos = to_position_key(x, y);
                Cell* cell = new Cell();
                board_map[pos] = cell;
            }
        }
    }

    /**
     * @brief Checks if a given position is a valid position on the board.
     * 
     * @param x The x-coordinate of the position.
     * @param y The y-coordinate of the position.
     * @return true if the position is valid, false otherwise.
     */
    bool is_valid_position(int32 x, int32 y) {
        int32 pos = to_position_key(x, y);
        return is_valid_position(pos);
    }

    /**
     * @brief Gets a list of valid moves for a given position.
     * 
     * @param pos The position for which to generate valid moves.
     * @return A list of valid moves as positions.
     */
    list<Position> get_valid_moves(Position& pos) {
        auto key = to_position_key(pos);
        auto moves = get_valid_moves(key);
        list<Position> pos_list = {};
        for (const int32 k : moves) {
            Position p = this->to_position(k);
            pos_list.push_front(p);
        }
        return pos_list;
    }

    /**
     * @brief Gets a list of valid moves for a given position key.
     * 
     * @param key The position key for which to generate valid moves.
     * @return A list of valid moves as position keys.
     */
    list<int32> get_valid_moves(int32 key) {
        return get_valid_moves(board_map, key);
    }

    /**
     * @brief Gets a list of valid moves for a given position key in a specific board configuration.
     * 
     * @param in_board The board configuration to use.
     * @param key The position key for which to generate valid moves.
     * @param skip_filter Whether to skip the filtering step to check if the move leaves the king in check.
     * @return A list of valid moves as position keys.
     */
    list<int32> get_valid_moves(map<int32, Cell*>& in_board, int32 key, bool skip_filter = false) {
        Cell* cell = in_board[key];
        list<int32> l = {};
        switch (cell->get_piece_type()) {
            case Cell::PieceType::none:
                break;
            case Cell::PieceType::pawn:
                add_pawn_moves(in_board, l, key, cell);
                break;
            case Cell::PieceType::bishop:
                add_bishop_moves(in_board, l, key, cell);
                break;
            case Cell::PieceType::knight:
                add_knight_moves(in_board, l, key, cell);
                break;
            case Cell::PieceType::rook:
                add_rook_moves(in_board, l, key, cell);
                break;
            case Cell::PieceType::queen:
                add_queen_moves(in_board, l, key, cell);
                break;
            case Cell::PieceType::king:
                add_king_moves(in_board, l, key, cell);
                break;
        }

        list<int32> filtered_list = {};
        auto color_pieces = get_piece_keys(cell->get_piece_color());
        int32 king_key = -1;
        for (auto piece_key : color_pieces) {
            if (in_board[piece_key]->get_piece_type() == Cell::PieceType::king) {
                king_key = piece_key;
                break;
            }
        }
        if (skip_filter || king_key == -1) {
            return l;
        }
        for (int32 k : l) {
            auto board_copy = copy_board_map();
            Position start = to_position(key);
            Position goal = to_position(k);

            move_piece(board_copy, start, goal);
            auto final_king_key = king_key;
            if (cell->get_piece_type() == Cell::PieceType::king) {
                final_king_key = k;
            }
            if (!can_be_captured(board_copy, final_king_key)) {
                filtered_list.push_front(k);
            }

            clear_board_map(board_copy);
        }
        return filtered_list;
    }

    /**
     * @brief Gets a list of position keys for all pieces of a given color.
     * 
     * @param pc The color of the pieces.
     * @return A list of position keys for all pieces of the given color.
     */
    list<int32> get_piece_keys(Cell::PieceColor pc) {
        return get_piece_keys(board_map, pc);
    }

    /**
     * @brief Gets a list of position keys for all pieces of a given color in a specific board configuration.
     * 
     * @param in_board The board configuration to use.
     * @param pc The color of the pieces.
     * @return A list of position keys for all pieces of the given color.
     */
    list<int32> get_piece_keys(map<int32, Cell*>& in_board, Cell::PieceColor pc) {
        list<int32> l = {};
        for(const auto& [key, cell] : in_board) {
            if (cell->get_piece_color() == pc) {
                l.push_front(key);
            }
        }
        return l;
    }

    /**
     * @brief Gets a list of position keys for all pieces of a given color that have valid moves.
     * 
     * @param pc The color of the pieces.
     * @param skip_filter Whether to skip the filtering step to check if the move leaves the king in check.
     * @return A list of position keys for all pieces of the given color that have valid moves.
     */
    list<int32> get_all_piece_move_keys(Cell::PieceColor pc, bool skip_filter = false) {
        return get_all_piece_move_keys(board_map, pc, skip_filter);
    }

    /**
     * @brief Gets a list of position keys for all pieces of a given color that can move to a specific target position.
     * 
     * @param target The target position key.
     * @param pc The color of the pieces.
     * @return A list of position keys for all pieces of the given color that can move to the target position.
     */
    list<int32> get_possible_move_sources(int32 target, Cell::PieceColor pc) {
        return get_possible_move_sources(board_map, target, pc);
    }

    /**
     * @brief Gets a list of position keys for all pieces of a given color that can move to a specific target position.
     * 
     * @param in_board The map representing the chessboard.
     * @param target The target position key.
     * @param pc The color of the pieces.
     * @return A list of position keys for all pieces of the given color that can move to the target position.
     */
    list<int32> get_possible_move_sources(map<int32, Cell*>& in_board, int32 target, Cell::PieceColor pc) {
        list<int32> l = {};
        auto all_moves = get_all_piece_move_keys(in_board, pc, true);
        for (auto move : all_moves) {
            auto moves = get_valid_moves(in_board, move, true);
            auto k = find(begin(moves), end(moves), target);
            if (k != end(moves)) {
                l.push_front(move);
            }
        }
        return l;
    }

    /**
     * @brief Converts a position key to a Position object.
     * 
     * @param key The position key to convert.
     * @return The Position object corresponding to the position key.
     */
    Position to_position(int32 key) {
        Position pos = Position{get_x(key), get_y(key)};
        return pos;
    }

    /**
     * @brief Checks if there are any valid moves for a given color.
     * 
     * @param pc The color of the pieces.
     * @return true if there are valid moves, false otherwise.
     */
    bool are_there_valid_moves(Cell::PieceColor pc) {
        return are_there_valid_moves(board_map, pc);
    }

    /**
     * @brief Checks if there are any valid moves for a given color.
     * 
     * @param in_board The map representing the chessboard.
     * @param pc The color of the pieces.
     * @return true if there are valid moves, false otherwise.
     */
    bool are_there_valid_moves(map<int32, Cell*>& in_board, Cell::PieceColor pc) {
        auto all_moves = get_all_piece_move_keys(in_board, pc);
        return all_moves.size() > 0;
    }

    /**
     * @brief Moves a piece from a start position to a goal position.
     * 
     * @param start The start position.
     * @param goal The goal position.
     * @return true if the move was successful, false otherwise.
     */
    bool move_piece(Position& start, Position& goal) {
        return move_piece(board_map, start, goal);
    }

    /**
     * @brief Moves a piece from a start position to a goal position.
     * 
     * @param in_board The map representing the chessboard.
     * @param start The start position.
     * @param goal The goal position.
     * @return true if the move was successful, false otherwise.
     */
    bool move_piece(map<int32, Cell*>& in_board, Position& start, Position& goal) {
        bool is_main_board = &in_board == &board_map;

        #if WITH_EDITOR
        // disabled for now, might be costly
        // UE_LOG(LogChessEngine, Log, TEXT("%s: Move piece from (%d, %d) to (%d, %d)"), is_main_board ? TEXT("MAIN") : TEXT("INNER"), start.x, start.y, goal.x, goal.y);
        #endif

        int32 sp = to_position_key(start);
        if (is_valid_position(in_board, sp) && is_valid_position(in_board, goal)) {
            Cell::PieceType pt = in_board[sp]->get_piece_type();
            Cell::PieceColor pc = in_board[sp]->get_piece_color();
            in_board[sp]->remove_piece();
            set_piece(in_board, goal, pt, pc);
        }
        return true;
    }

    /**
     * @brief Sets a piece at a given position.
     * 
     * @param pos The position to set the piece at.
     * @param pt The type of the piece.
     * @param pc The color of the piece.
     * @return true if the piece was set successfully, false otherwise.
     */
    bool set_piece(Position& pos, Cell::PieceType pt, Cell::PieceColor pc) {
        return set_piece(board_map, pos, pt, pc);
    }

    /**
     * @brief Sets a piece at a given position.
     * 
     * @param in_board The map representing the chessboard.
     * @param pos The position to set the piece at.
     * @param pt The type of the piece.
     * @param pc The color of the piece.
     * @return true if the piece was set successfully, false otherwise.
     */
    bool set_piece(map<int32, Cell*>& in_board, Position& pos, Cell::PieceType pt, Cell::PieceColor pc) {
        int32 key = to_position_key(pos);
        if (!is_valid_position(in_board, key)) {
            return false;
        }
        in_board[key]->set_piece(pt, pc);
        return false;
    }

    /**
     * @brief Checks if a piece at a given position can be captured by an opponent.
     * 
     * @param pos The position of the piece.
     * @return true if the piece can be captured, false otherwise.
     */
    bool can_be_captured(Position& pos) {
        return can_be_captured(board_map, pos);
    }

    /**
     * @brief Checks if a piece at a given position can be captured by an opponent.
     * 
     * @param in_board The map representing the chessboard.
ы     * @param pos The position of the piece.
     * @return true if the piece can be captured, false otherwise.
     */
    bool can_be_captured(map<int32, Cell*>& in_board, Position& pos) {
        int32 key = to_position_key(pos);
        return can_be_captured(in_board, key);
    }

    /**
     * @brief Evaluates the current board state and returns a score.
     * 
     * @return The score of the current board state.
     */
    int32 evaluate() {
        return evaluate(board_map);
    }

    /**
     * @brief Evaluates the target board state and returns a score.
     * 
     * @param in_board The map representing the chessboard.
     * @return The score of the current board state.
     */
    int32 evaluate(map<int32, Cell*>& in_board)
    {
        // set up some scoring for figures
        int32 score = 0;

        // get all pieces
        auto white_pieces = get_piece_keys(in_board, Cell::PieceColor::white);
        auto black_pieces = get_piece_keys(in_board, Cell::PieceColor::black);

        // count each piece with modifier based on its type
        // whites are positive while blacks are negative
        // check is severely punished
        for (auto piece_key : white_pieces)
        {
            if (in_board[piece_key]->get_piece_type() == Cell::PieceType::king)
            {
                if (can_be_captured(in_board, piece_key))
                {
                    score -= piece_values[Cell::PieceType::king];
                }
            }
            else
            {
                score += piece_values[in_board[piece_key]->get_piece_type()];
            }
        }
        for (auto piece_key : black_pieces)
        {
            if (in_board[piece_key]->get_piece_type() == Cell::PieceType::king)
            {
                if (can_be_captured(in_board, piece_key))
                {
                    score += piece_values[Cell::PieceType::king];
                }
            }
            else
            {
                score -= piece_values[in_board[piece_key]->get_piece_type()];
            }
        }

        return score;
    }

    map<int32, Cell*> copy_board_map() {
        map<int32, Cell*> board_map_copy = {};
        for (const auto& [key, cell] : this->board_map) {
            board_map_copy[key] = new Cell(*cell);
        }
        return board_map_copy;
    }

    map<int32, Cell*> copy_board_map(map<int32, Cell*>& in_board) {
        map<int32, Cell*> board_map_copy = {};
        for (const auto& [key, cell] : in_board) {
            board_map_copy[key] = new Cell(*cell);
        }
        return board_map_copy;
    }

    void clear_board_map(std::map<int32, Cell*>& in_board) {
        for (auto& pair : in_board) {
            delete pair.second;  // Deletes the pointer
        }
        in_board.clear();  // Clears the map
    }

    map<int32, Cell*> board_map;

private:
    using TMoveFn = int32 (*)(const int32);

    static const int32 median = 5;
    static const int32 max = 10;
    static const int32 step_x = 1 << 8;
    const vector<int32> white_pawn_cell_keys = {256, 513, 770, 1027, 1284, 1539, 1794, 2049, 2304};
    const vector<int32> black_pawn_cell_keys = {262, 518, 774, 1030, 1286, 1542, 1798, 2054, 2310};

    /**
     * @brief Converts x and y coordinates to a position key.
     * 
     * @param x The x-coordinate.
     * @param y The y-coordinate.
     * @return The position key.
     */
    inline int32 to_position_key(int32 x, int32 y) {
        return (x << 8) + y;
    }

    /**
     * @brief Converts a Position object to a position key.
     * 
     * @param pos The Position object.
     * @return The position key.
     */
    inline int32 to_position_key(Position pos) {
        return to_position_key(pos.x, pos.y);
    }

    /**
     * @brief Checks if a given position key is a valid position on the board.
     * 
     * @param key The position key.
     * @return true if the position is valid, false otherwise.
     */
    inline bool is_valid_position(int32 key) {
        return is_valid_position(board_map, key);
    }

    /**
     * @brief Checks if a given position key is a valid position on a specific board configuration.
     * 
     * @param in_board The board configuration to use.
     * @param key The position key.
     * @return true if the position is valid, false otherwise.
     */
    inline bool is_valid_position(map<int32, Cell*>& in_board, int32 key) {
        return in_board.find(key) != in_board.end();
    }

    /**
     * @brief Checks if a given position is a valid position on the board.
     * 
     * @param pos The position to check.
     * @return true if the position is valid, false otherwise.
     */
    inline bool is_valid_position(Position& pos) {
        return is_valid_position(board_map, pos);
    }

    /**
     * @brief Checks if the given position is a valid position on the board.
     *
     * This function checks if the given position is a valid position on the board.
     *
     * @param in_board The map representing the chessboard.
     * @param pos The position to check.
     * @return True if the position is valid, false otherwise.
     */
    inline bool is_valid_position(map<int32, Cell*>& in_board, Position& pos) {
        return is_valid_position(in_board, to_position_key(pos));
    }

    /**
     * @brief Adds all possible moves for a pawn to the given list of cells.
     *
     * This function calculates all possible moves for a pawn from the given key position and adds them to the provided list of cells.
     *
     * @param in_board The map representing the chessboard.
     * @param l The list of cells to which the pawn moves will be added.
     * @param key The key position from which the pawn moves will be calculated.
     * @param cell The cell object representing the pawn.
     */
    void add_pawn_moves(map<int32, Cell*>& in_board, list<int32>& l, int32 key, Cell* cell) {
        TMoveFn fn_move, fn_take_1, fn_take_2;
        switch (cell->get_piece_color()) {
            case Cell::PieceColor::white:
                fn_move = &Board::move_vertically_up;
                fn_take_1 = &Board::move_horizontally_top_left;
                fn_take_2 = &Board::move_horizontally_top_right;
                break;
            case Cell::PieceColor::black:
                fn_move = &Board::move_vertically_down;
                fn_take_1 = &Board::move_horizontally_bottom_left;
                fn_take_2 = &Board::move_horizontally_bottom_right;
                break;
            default:
                return;
        }
        int32 move = fn_move(key);
        if (is_valid_position(in_board, move)) {
            add_if_valid(in_board, l, move, cell, false);
            if (!in_board[move]->has_piece() && is_initial_pawn_cell(key, cell)) {
                add_if_valid(in_board, l, fn_move(move), cell, false);
            }
        }
        int32 take = fn_take_1(key);
        add_pawn_take_if_valid(in_board, l, take, cell);
        take = fn_take_2(key);
        add_pawn_take_if_valid(in_board, l, take, cell);
    }

    /**
     * @brief Adds a pawn take move to the given list of cells if it is a valid move.
     *
     * This function adds a pawn take move to the given list of cells if it is a valid move.
     *
     * @param in_board The map representing the chessboard.
     * @param l The list of cells to which the pawn take move will be added.
     * @param key The key position of the take move.
     * @param cell The cell object representing the pawn.
     */
    void add_pawn_take_if_valid(map<int32, Cell*>& in_board, list<int32>& l, int32 key, Cell* cell) {
        if (is_valid_position(in_board, key) && in_board[key]->has_piece_of_opposite_color(cell)) {
            l.push_front(key);
        }
    }

    /**
     * @brief Checks if the given key represents an initial pawn cell.
     *
     * This function checks if the given key represents an initial pawn cell.
     *
     * @param key The key position to check.
     * @param cell The cell object representing the pawn.
     * @return True if the key represents an initial pawn cell, false otherwise.
     */
    bool is_initial_pawn_cell(const int32 key, Cell* cell) {
        const vector<int32>* cell_keys;
        switch (cell->get_piece_color()) {
            case Cell::PieceColor::white:
                cell_keys = &white_pawn_cell_keys;
                break;
            case Cell::PieceColor::black:
                cell_keys = &black_pawn_cell_keys;
                break;
            default:
                return false;
        }
        auto arr_end = end(*cell_keys);
        auto k = find(begin(*cell_keys), arr_end, key);
        return k != arr_end;
    }

    /**
     * @brief Adds all possible moves for a bishop to the given list of cells.
     *
     * This function calculates all possible moves for a bishop from the given key position and adds them to the provided list of cells.
     *
     * @param in_board The map representing the chessboard.
     * @param l The list of cells to which the bishop moves will be added.
     * @param key The key position from which the bishop moves will be calculated.
     * @param cell The cell object representing the bishop.
     */
    void add_bishop_moves(map<int32, Cell*>& in_board, list<int32>& l, int32 key, Cell* cell) {
        TMoveFn fns[6] = { &move_diagonally_top_right
                         , &move_diagonally_top_left
                         , &move_diagonally_bottom_right
                         , &move_diagonally_bottom_left
                         , &move_diagonally_right
                         , &move_diagonally_left
                         };
        add_valid_moves(in_board, l, key, fns, 6, cell);
    }

    /**
     * @brief Adds all possible knight moves to the given list of cells.
     *
     * This function calculates all possible knight moves from the given key position and adds them to the provided list of cells.
     *
     * @param in_board The map representing the chessboard.
     * @param l The list of cells to which the knight moves will be added.
     * @param key The key position from which the knight moves will be calculated.
     * @param cell The cell object representing the knight.
     */
    void add_knight_moves(map<int32, Cell*>& in_board, list<int32>& l, int32 key, Cell* cell) {
        int32 pos;
        pos = move_vertically_up(move_vertically_up(key));
        add_if_valid(in_board, l, move_horizontally_top_right(pos), cell, true);
        add_if_valid(in_board, l, move_horizontally_top_left(pos), cell, true);

        pos = move_vertically_down(move_vertically_down(key));
        add_if_valid(in_board, l, move_horizontally_bottom_right(pos), cell, true);
        add_if_valid(in_board, l, move_horizontally_bottom_left(pos), cell, true);

        pos = move_horizontally_top_right(move_horizontally_top_right(key));
        add_if_valid(in_board, l, move_vertically_up(pos), cell, true);
        add_if_valid(in_board, l, move_horizontally_bottom_right(pos), cell, true);

        pos = move_horizontally_bottom_right(move_horizontally_bottom_right(key));
        add_if_valid(in_board, l, move_vertically_down(pos), cell, true);
        add_if_valid(in_board, l, move_horizontally_top_right(pos), cell, true);

        pos = move_horizontally_bottom_left(move_horizontally_bottom_left(key));
        add_if_valid(in_board, l, move_vertically_down(pos), cell, true);
        add_if_valid(in_board, l, move_horizontally_top_left(pos), cell, true);

        pos = move_horizontally_top_left(move_horizontally_top_left(key));
        add_if_valid(in_board, l, move_vertically_up(pos), cell, true);
        add_if_valid(in_board, l, move_horizontally_bottom_left(pos), cell, true);
    }

    /**
     * @brief Adds all possible moves for a rook to the given list of cells.
     *
     * This function calculates all possible moves for a rook from the given key position and adds them to the provided list of cells.
     *
     * @param in_board The map representing the chessboard.
     * @param l The list of cells to which the rook moves will be added.
     * @param key The key position from which the rook moves will be calculated.
     * @param cell The cell object representing the rook.
     */
    void add_rook_moves(map<int32, Cell*>& in_board, list<int32>& l, int32 key, Cell* cell) {
        TMoveFn fns[6] = { &move_horizontally_top_right
                         , &move_horizontally_top_left
                         , &move_horizontally_bottom_right
                         , &move_horizontally_bottom_left
                         , &move_vertically_up
                         , &move_vertically_down
                         };
        add_valid_moves(in_board, l, key, fns, 6, cell);
    }

    /**
     * @brief Adds all possible moves for a queen to the given list of cells.
     *
     * This function calculates all possible moves for a queen from the given key position and adds them to the provided list of cells.
     *
     * @param in_board The map representing the chessboard.
     * @param l The list of cells to which the queen moves will be added.
     * @param key The key position from which the queen moves will be calculated.
     * @param cell The cell object representing the queen.
     */
    void add_queen_moves(map<int32, Cell*>& in_board, list<int32>& l, int32 key, Cell* cell) {
        add_bishop_moves(in_board, l, key, cell);
        add_rook_moves(in_board, l, key, cell);
    }

    /**
     * @brief Adds all possible moves for a king to the given list of cells.
     *
     * This function calculates all possible moves for a king from the given key position and adds them to the provided list of cells.
     *
     * @param in_board The map representing the chessboard.
     * @param l The list of cells to which the king moves will be added.
     * @param key The key position from which the king moves will be calculated.
     * @param cell The cell object representing the king.
     */
    void add_king_moves(map<int32, Cell*>& in_board, list<int32>& l, int32 key, Cell* cell) {
        add_if_valid(in_board, l, move_vertically_up(key), cell, true);
        add_if_valid(in_board, l, move_vertically_down(key), cell, true);
        add_if_valid(in_board, l, move_horizontally_top_right(key), cell, true);
        add_if_valid(in_board, l, move_horizontally_top_left(key), cell, true);
        add_if_valid(in_board, l, move_horizontally_bottom_right(key), cell, true);
        add_if_valid(in_board, l, move_horizontally_bottom_left(key), cell, true);
        add_if_valid(in_board, l, move_diagonally_top_right(key), cell, true);
        add_if_valid(in_board, l, move_diagonally_top_left(key), cell, true);
        add_if_valid(in_board, l, move_diagonally_bottom_right(key), cell, true);
        add_if_valid(in_board, l, move_diagonally_bottom_left(key), cell, true);
        add_if_valid(in_board, l, move_diagonally_right(key), cell, true);
        add_if_valid(in_board, l, move_diagonally_left(key), cell, true);
    }

    /**
     * @brief Adds valid moves to the given list based on the provided move functions.
     * 
     * This function iterates through the given move functions and generates valid moves
     * for the given key on the chessboard. It checks each move position and determines
     * whether it is a valid move or not based on the current state of the board.
     * 
     * @param in_board The chessboard represented as a map of cell positions to cell objects.
     * @param l The list to which the valid move positions will be added.
     * @param key The key representing the current position on the chessboard.
     * @param fns An array of move functions that generate the next position based on the current position.
     * @param fns_count The number of move functions in the array.
     * @param cell The cell object representing the current position on the chessboard.
     */
    void add_valid_moves(map<int32, Cell*>& in_board, list<int32>& l, const int32 key, TMoveFn fns[], int32 fns_count, Cell* cell) {
        int32 current_pos;
        for (int32 i = 0; i < fns_count; i++) {
            TMoveFn fn = fns[i];
            current_pos = fn(key);
            while (is_valid_position(in_board, current_pos)) {
                Cell* c = in_board[current_pos];
                if (c->has_piece()) {
                    if (c->has_piece_of_same_color(cell)) {
                        // cannot take a piece of the same color and cannot move further
                        break;
                    } else {
                        // can take a piece of the opposite color but cannot move further
                        l.push_front(current_pos);
                        break;
                    }
                } else {
                    // empty cell, can continue moving
                    l.push_front(current_pos);
                    current_pos = fn(current_pos);
                }
            }
        }
    }

    /**
     * Adds a valid position to the given list if it satisfies certain conditions.
     *
     * @param in_board The map representing the chess board.
     * @param l The list to which the valid position will be added.
     * @param key The key representing the position on the board.
     * @param cell The cell object representing the current cell.
     * @param can_take A boolean indicating whether the current cell can take a piece.
     */
    inline void add_if_valid(map<int32, Cell*>& in_board, list<int32>& l, int32 key, Cell* cell, bool can_take) {
        if (is_valid_position(in_board, key)) {
            Cell* c = in_board[key];
            if (c->has_piece()) {
                if (c->has_piece_of_opposite_color(cell) && can_take) {
                    l.push_front(key);
                }
            } else {
                l.push_front(key);
            }
        }
    }

    static inline int32 move_vertically_up(const int32 key) {
        return key + 1; // x, y+1
    }

    static inline int32 move_vertically_down(const int32 key) {
        return key - 1; // x, y-1
    }

    static int32 move_horizontally_top_right(const int32 key) {
        if (get_x(key) < median) {
            return key + step_x + 1; // x+1, y+1
        } else {
            return key + step_x; // x+1, y
        }
    }

    static int32 move_horizontally_top_left(const int32 key) {
        if (get_x(key) > median) {
            return key - step_x + 1; // x-1, y+1
        } else {
            return key - step_x; // x-1, y
        }
    }

    static int32 move_horizontally_bottom_right(const int32 key) {
        if (get_x(key) < median) {
            return key + step_x; // x+1, y
        } else {
            return key + step_x - 1; // x+1, y-1
        }
    }

    static int32 move_horizontally_bottom_left(const int32 key) {
        if (get_x(key) > median) {
            return key - step_x; // x-1, y
        } else {
            return key - step_x - 1; // x-1, y-1
        }
    }

    static int32 move_diagonally_top_right(const int32 key) {
        if (get_x(key) < median) {
            return key + step_x + 2; // x+1, y+2
        } else {
            return key + step_x + 1; // x+1, y+1
        }
    }

    static int32 move_diagonally_top_left(const int32 key) {
        if (get_x(key) > median) {
            return key - step_x + 2; // x-1, y+2
        } else {
            return key - step_x + 1; // x-1, y+1
        }
    }

    static int32 move_diagonally_bottom_right(const int32 key) {
        if (get_x(key) < median) {
            return key + step_x - 1; // x+1, y-1
        } else {
            return key + step_x - 2; // x+1, y-2
        }
    }

    static int32 move_diagonally_bottom_left(const int32 key) {
        if (get_x(key) > median) {
            return key - step_x - 1; // x-1, y-1
        } else {
            return key - step_x - 2; // x-1, y-2
        }
    }

    static int32 move_diagonally_right(const int32 key) {
        int32 x = get_x(key);
        if (x % 2 == 0) {
            if (x == median - 1) {
                return key + step_x*2; // x+2, y
            } else if (x < median) {
                return key + step_x*2 + 1; // x+2, y+1
            } else {
                return key + step_x*2 - 1; // x+2, y-1
            }
        } else {
            if (x < median) {
                return key + step_x*2 + 1; // x+2, y+1
            } else {
                return key + step_x*2 - 1; // x+2, y-1
            }
        }
    }

    static int32 move_diagonally_left(const int32 key) {
        int32 x = get_x(key);
        if (x % 2 == 0) {
            if (x == median + 1) {
                return key - step_x*2; // x-2, y
            } else if (x < median) {
                return key - step_x*2 - 1; // x-2, y-1
            } else {
                return key - step_x*2 + 1; // x-2, y+1
            }
        } else {
            if (x <= median) {
                return key - step_x*2 - 1; // x-2, y-1
            } else {
                return key - step_x*2 + 1; // x-2, y+1
            }
        }
    }

    static int32 ping(const int32 key) {
        return key;
    }

    static inline int32 get_x(const int32 key) {
        return key >> 8;
    }

    static inline int32 get_y(const int32 key) {
        return key & 0xFF;
    }

    /**
     * Retrieves all possible move keys for a given piece color on the chess board.
     * 
     * @param in_board The chess board represented as a map of cell keys to cell pointers.
     * @param pc The piece color for which to retrieve the move keys.
     * @param skip_filter Flag indicating whether to skip the move filter.
     * @return A list of all possible move keys for the given piece color.
     */
    list<int32> get_all_piece_move_keys(map<int32, Cell*>& in_board, Cell::PieceColor pc, bool skip_filter = false) {
        list<int32> all_moves = {};
        auto all_piece_keys = get_piece_keys(in_board, pc);
        for (int32 key : all_piece_keys) {
            auto moves = get_valid_moves(in_board, key, skip_filter);
            all_moves.insert(all_moves.end(), moves.begin(), moves.end());
        }
        return all_moves;
    }

    /**
     * Determines whether a chess piece with the given key can be captured.
     *
     * @param key The key of the chess piece.
     * @return True if the chess piece can be captured, false otherwise.
     */
    bool can_be_captured(const int32 key) {
        return can_be_captured(board_map, key);
    }

    /**
     * Checks if a chess piece can be captured by the opponent.
     *
     * @param in_board The chess board represented as a map of cell keys to cell pointers.
     * @param key The key of the cell representing the chess piece to be checked.
     * @return True if the chess piece can be captured, false otherwise.
     */
    bool can_be_captured(map<int32, Cell*>& in_board, const int32 key) {
        Cell::PieceColor pc = in_board[key]->get_opposite_color();
        auto all_moves = get_all_piece_move_keys(in_board, pc, true);
        auto k = find(begin(all_moves), end(all_moves), key);
        return k != end(all_moves);
    }
};
