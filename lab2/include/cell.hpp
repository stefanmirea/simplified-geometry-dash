/* Stefan-Gabriel Mirea - 331CC */

/* Declararea clasei Cell, pentru o celula din matrice. */
#ifndef CELL_HPP_
#define CELL_HPP_

struct Cell {
    int row, col;

    Cell();
    Cell(int row, int col);
    bool operator==(const Cell &cell) const;
    bool operator!=(const Cell &cell) const;
};

#endif
