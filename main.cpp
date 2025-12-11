#include <termios.h>
#include <unistd.h>

#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>
#include <random>
#include <vector>

// consts
constexpr int DEFAULT_ROWS            = 10;
constexpr int DEFAULT_COLS            = 10;
constexpr int DEFAULT_BOMB_PERCENTAGE = 10;
constexpr int CELL_WIDTH              = 3;

class TerminalMode {
   public:
    TerminalMode() {
        if (!isatty(STDIN_FILENO)) {
            std::cerr << "Not a terminal.\n";
            exit(EXIT_FAILURE);
        }

        tcgetattr(STDIN_FILENO, &m_savedAttributes);

        struct termios tattr = m_savedAttributes;
        tattr.c_lflag &= ~(ICANON | ECHO);
        tattr.c_cc[VMIN]  = 1;
        tattr.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &tattr);
    }

    ~TerminalMode() { tcsetattr(STDIN_FILENO, TCSANOW, &m_savedAttributes); }

    char readCmd() {
        char buf = 0;
        if (read(STDIN_FILENO, &buf, 1) < 0) {
            perror("read()");
        }
        return buf;
    }

   private:
    struct termios m_savedAttributes;
};

enum class CellContent { Empty, Bomb };
enum class CellStatus { Closed, Opened, Flagged };

struct Cell {
    CellContent content = CellContent::Empty;
    CellStatus status   = CellStatus::Closed;
};

class Field {
   public:
    Field(int r, int c, int bp) : m_numRows(r), m_numCols(c) {
        if (bp < 0) {
            m_bombPercentage = 0;
        } else if (bp > 100) {
            m_bombPercentage = 100;
        } else {
            m_bombPercentage = bp;
        }
        m_cursorCol = 0;
        m_cursorRow = 0;
        m_field.resize(m_numRows, std::vector<Cell>(m_numCols));
    }

    void refreshDisplay() {
        std::cout << "\033[" << getNumRows() << "A";
        std::cout << "\033[" << (CELL_WIDTH * getNumCols()) << "D";
    }

    void print(bool gameOver) {
        for (int i = 0; i < m_numRows; ++i) {
            for (int j = 0; j < m_numCols; ++j) {
                Cell& currentCell = getCell(i, j);
                bool atCursorCell = isCursorCell(i, j);

                if (atCursorCell) {
                    std::cout << "[";
                } else {
                    std::cout << " ";
                }

                char displayChar = '.';

                if (gameOver && currentCell.content == CellContent::Bomb) {
                    displayChar = '@';
                } else if (currentCell.status == CellStatus::Opened) {
                    if (currentCell.content != CellContent::Bomb) {
                        int neighbors = countNeighbors(i, j);
                        if (neighbors > 0) {
                            std::cout << neighbors;
                            displayChar = '\0';
                        } else {
                            displayChar = ' ';
                        }
                    }
                } else if (currentCell.status == CellStatus::Flagged) {
                    displayChar = '?';
                } else {
                    displayChar = '.';
                }

                if (displayChar != '\0') {
                    std::cout << displayChar;
                }

                if (atCursorCell) {
                    std::cout << "]";
                } else {
                    std::cout << " ";
                }
            }
            std::cout << std::endl;
        }
    }

    bool isAroundCursor(int row, int col) {
        return abs(row - m_cursorRow) <= 1 && abs(col - m_cursorCol) <= 1;
    }

    void randomize() {
        m_numBombs = m_numRows * m_numCols * m_bombPercentage / 100;
        for (int i = 1; i <= m_numBombs; ++i) {
            while (true) {
                int randomFlatIndex = getRandomCellIndex();
                int random_row      = randomFlatIndex / m_numCols;
                int random_col      = randomFlatIndex % m_numCols;
                if (!isAroundCursor(random_row, random_col)) {
                    Cell& cell = getCell(random_row, random_col);
                    if (cell.content != CellContent::Bomb) {
                        cell.content = CellContent::Bomb;
                        break;
                    }
                }
            }
        }
    }

    int getRandomCellIndex() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, m_numRows * m_numCols - 1);
        return dis(gen);
    }

    bool insideField(int row, int col) {
        return row >= 0 && row < m_numRows && col >= 0 && col < m_numCols;
    }

    Cell& getCell(int row, int col) {
        assert(insideField(row, col));
        return m_field[row][col];
    }

    int countNeighbors(int row, int col) {
        int counter = 0;
        for (int drow = -1; drow <= 1; ++drow) {
            for (int dcol = -1; dcol <= 1; ++dcol) {
                if (drow == 0 && dcol == 0) {
                    continue;
                }
                if (insideField(row + drow, col + dcol)) {
                    Cell& currentCell = getCell(row + drow, col + dcol);
                    if (currentCell.content == CellContent::Bomb) {
                        ++counter;
                    }
                }
            }
        }
        return counter;
    }

    bool isCursorCell(int row, int col) {
        return row == m_cursorRow && col == m_cursorCol;
    }

    void moveCursor(int dRow, int dCol) {
        m_cursorRow += dRow;
        m_cursorCol += dCol;
        if (m_cursorRow < 0) {
            m_cursorRow = 0;
        }
        if (m_cursorRow >= m_numRows) {
            m_cursorRow = m_numRows - 1;
        }

        if (m_cursorCol < 0) {
            m_cursorCol = 0;
        }
        if (m_cursorCol >= m_numCols) {
            m_cursorCol = m_numCols - 1;
        }
    }

    void flagCell() {
        Cell& cursorCell = getCell(m_cursorRow, m_cursorCol);
        if (cursorCell.status == CellStatus::Flagged) {
            cursorCell.status = CellStatus::Closed;
        } else if (cursorCell.status == CellStatus::Closed) {
            cursorCell.status = CellStatus::Flagged;
        }
    }

    bool openCell() {
        Cell& cursorCell = getCell(m_cursorRow, m_cursorCol);
        if (cursorCell.status != CellStatus::Closed) {
            return false;
        }
        if (cursorCell.content == CellContent::Bomb) {
            cursorCell.status = CellStatus::Opened;
            return true;
        }
        openAdjacentCells(m_cursorRow, m_cursorCol);
        return false;
    }

    void openAdjacentCells(int row, int col) {
        if (!insideField(row, col)) {
            return;
        }
        Cell& currentCell = getCell(row, col);
        if (currentCell.status == CellStatus::Opened) {
            return;
        }
        if (currentCell.content == CellContent::Bomb) {
            return;
        }
        currentCell.status = CellStatus::Opened;
        if (countNeighbors(row, col) == 0) {
            for (int drow = -1; drow <= 1; ++drow) {
                for (int dcol = -1; dcol <= 1; ++dcol) {
                    if (drow != 0 || dcol != 0)
                        openAdjacentCells(row + drow, col + dcol);
                }
            }
        }
    }

    bool checkWin() {
        int numFlagged = 0;
        for (int i = 0; i < m_numRows; ++i) {
            for (int j = 0; j < m_numCols; ++j) {
                Cell& cell = getCell(i, j);
                if (cell.content == CellContent::Bomb) {
                    if (cell.status != CellStatus::Flagged) {
                        return false;
                    } else {
                        numFlagged++;
                    }
                }
            }
        }
        return numFlagged == m_numBombs;
    }

    int getNumRows() { return m_numRows; }
    int getNumCols() { return m_numCols; }

   private:
    int m_numRows, m_numCols;
    int m_cursorRow, m_cursorCol;
    int m_bombPercentage, m_numBombs;
    std::vector<std::vector<Cell>> m_field;
};

void handleGameEnd(std::shared_ptr<Field> field, bool won) {
    field->refreshDisplay();
    field->print(!won);
    std::cout << (won ? "You win!\n" : "Game Over\n");
    return;
}

int main() {
    auto terminal  = std::make_unique<TerminalMode>();
    bool firstStep = true;
    bool quit      = false;

    auto field = std::make_shared<Field>(DEFAULT_ROWS, DEFAULT_COLS,
                                         DEFAULT_BOMB_PERCENTAGE);
    field->print(false);

    while (!quit) {
        char cmd = terminal->readCmd();
        switch (cmd) {
            case 'q':
                quit = true;
                break;
            case 'w':
                field->moveCursor(-1, 0);
                break;
            case 'a':
                field->moveCursor(0, -1);
                break;
            case 's':
                field->moveCursor(1, 0);
                break;
            case 'd':
                field->moveCursor(0, 1);
                break;
            case 'f':
                field->flagCell();
                if (field->checkWin()) {
                    handleGameEnd(field, true);
                    quit = true;
                }
                break;
            case ' ':
                if (firstStep) {
                    field->randomize();
                    firstStep = false;
                }
                if (field->openCell()) {
                    handleGameEnd(field, false);
                    quit = true;
                }
                break;
        }

        if (!quit && cmd != 'q') {
            field->refreshDisplay();
            field->print(false);
        }
    }

    return 0;
}
