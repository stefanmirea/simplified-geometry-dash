/* Stefan-Gabriel Mirea - 331CC */

/* Implementarea clasei Cell. */
#include <cell.hpp>

Cell::Cell() : row(0), col(0) {}

Cell::Cell(int row, int col) : row(row), col(col) {}

bool Cell::operator==(const Cell &cell) const
{
    return row == cell.row && col == cell.col;
}

bool Cell::operator!=(const Cell &cell) const
{
    return !(*this == cell);
}
