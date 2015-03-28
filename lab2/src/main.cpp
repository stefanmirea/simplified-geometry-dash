/* Stefan-Gabriel Mirea, 331CC */

/* Singurul fisier modificat fata de scheletul de laborator: contine implementarile functiilor
 * DrawingWindow::init(), DrawingWindow::onIdle(), DrawingWindow::onKey(),
 * DrawingWindow::onReshape(), impreuna cu multe alte functii ajutatoare.
 */
#include <DrawingWindow.h>
#include <Visual2D.h>
#include <Transform2D.h>
#include <Line2D.h>
#include <Rectangle2D.h>
#include <Circle2D.h>
#include <Polygon2D.h>
#include <cell.hpp>

#include <iostream>
#include <fstream>
#include <list>
#include <ctime>
#include <algorithm>
#include <windows.h>

#define PI           3.14159265358979323846f

#define BACK_RATIO   0.8f /* variatia pozitiei fundalului / variatia pozitiei "camerei" */
#define MAX_TRAJ     20   /* numarul maxim de liniute ale traiectoriei afisate in timpul unei
                           * sarituri automate */
#define MAX_CIRCLE   30   /* timpul de viata al cercului din cadrul efectelor vizuale */

#define EXP_LINES    200  /* numarul de liniute din simularea exploziei jucatorului */
#define EXP_LEN      13   /* lungimea unei liniute din explozie */
#define EXP_TIME     10   /* cat dureaza explozia */

#define ROUND_DELAY  50   /* cat dureaza pana la reinceperea de la ultimul checkpoint */
#define FLAGS_NEEDED 5    /* cate stegulete trebuie colectate pentru castig (minim 1) */

/* directiile de intersectie cu platformele */
#define NO_INT       0
#define INT_UP       1
#define INT_DOWN     2
#define INT_RIGHT    3

/* starea */
#define SEATED       0
#define JUMPING      1
#define FALLING      2

/* cat timp (in apeluri onIdle()) mai are efect apasarea lui SPACE, care a fost ignorata pentru ca
 * jucatorul era in aer */
#define SPACE_DELAY  10

/* toleranta pentru compararea numerelor float */
#define EPS          1e-20

using namespace std;

/* Variabile globale (am folosit multe, pentru ca trebuiau completate doar anumite metode din clasa
 * DrawingWindow si nu era recomandata modificarea framework-ului) */

/* viteza jocului (ca patrate parcurse pe orizontala intre doua apeluri onIdle() succesive) */
static float speed;
/* contextele vizuale */
static Visual2D *script, *script_backgr, *game, *backgr1, *backgr2;
/* dreptunghiul pe care se afiseaza scorul si incercarile */
static Rectangle2D *script_rect;
/* latimea spatiului de joc, necesara pentru pozitia textelor */
static int game_width;
/* pozitia si rotatia jucatorului */
static Point2D player;
static float player_angle;
/* pozitia pe verticala de unde a inceput saritura curenta */
static float jump_start_y;
/* timpul la care a inceput saritura curenta */
static unsigned int jump_start_t;
/* punctul din stanga-jos al contextului vizual game */
static Point2D camera;
/* distanta pe Ox pana unde va incepe pamantul */
static float ground_offset;
/* pozitia de start din matrice */
static Cell player_cell;
/* coeficientii parabolei ce descrie miscarea in timpul unei sarituri / caderi */
static float coef_a, coef_b;
/* spune daca jucatorul va fi urmarit de camera (va fi intotdeauna, mai putin la inceputul
 * jocului) */
static bool follow_player;
/* numarul primei componente a pamantului / tavanului */
static int first_tile;
/* prima si ultima coloana din matricea de elemente, care sunt cuprinse in cadru */
static int first_seen_col, last_seen_col;
/* pozitiile in objects2D ale primului si ultimului element din scena */
static int first_elem_pos, last_elem_pos;
/* pozitiile in objects2D ale primei si ultimei liniute din traiectoria din timpul sariturii
 * automate */
static int first_traj_pos, last_traj_pos;
/* starea jucatorului: asezat / sarind / cazand */
static int state;
/* linia pe care jucatorul e asezat */
static int player_line;
/* spune daca jucatorul e asezat pe elemenul reprezentat de caracterul '_' (dreptunghi jos) */
static bool elevated;
/* spune daca jucatorul realizeaza o saritura automata */
static bool auto_jump;
/* cand a inceput actuala saritura automata (doar daca auto_jump == true) */
static unsigned int start_auto_jump;
/* cand s-a apasat ultima data SPACE fara rezultat (jucatorul fiind in aer) */
static unsigned int last_space;
/* ultimul element de jump de care s-a lovit (coloana depinde si de numarul de cate ori s-a repetat
 * harta) */
static Cell jump_elem;
/* timpul curet, in numar de apeluri ale functiei onIdle() */
static unsigned int idle_calls;
/* liniile fisierului cu harta */
static vector<string> lines;
/* toate culorile folosite */
static vector<Color *> colors;
/* obiectele ce intra in alcatuirea jucatorului respectiv a pamantului */
static vector<Object2D *> player_graphics, ground;
/* patratele din fundal */
static vector<vector<Object2D *>> background;
/* toate obiectele afisate */
static vector<vector<vector<Object2D *>>> objects;
/* varibile pentru retinerea elementelor de saritura automata vizibile (pentru scalari periodice) */
static list<Cell> auto_jump_list;
/* variabile pentru cercurile ce creeaza diferite efecte */
static int first_circle_pos, last_circle_pos;
static list<Object2D *> circles;
static list<Point2D> circle_positions;
static list<unsigned int> circles_time;
/* ultimul checkpoint atins */
static Cell last_checkpoint;
/* numarul de vieti */
static int lifes;
/* vector cu liniile si coloanele steguletelor colectate */
static vector<Cell> collected;
/* spune daca jucatorul a murit */
static bool dead;
/* variabile pentru efectul de explozie a jucatorului */
static int first_explosion_pos, last_explosion_pos;
static vector<float> explosion_angles;
/* timpul cand jucatorul a murit */
static unsigned int death_time;
/* spune daca e pauza sau nu */
static bool paused;
/* spune daca se asteapta apasarea unei taste */
static bool waiting_for_key;
/* true in caz de pierdere */
static bool game_over;
/* true in caz de castig */
static bool game_won;
/* matricea ce apare in cazul in care jocul e castigat */
static char won_matr_char[4][30] = {
    {0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1},
    {0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1},
    {0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0},
    {0, 0, 1, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1}};
static vector<vector<vector<Object2D *>>> won_matr_obj;
/* coloana din matricea de mai sus la care am ajuns cu afisarea */
static int won_column;
/* ultima coloana de obiecte care nu fac parte din won_matr_obj */
static int last_non_won_col;

/* returneaza {a / b} * b */
float real_modulo(const float a, const float b)
{
    if (a >= 0)
        return fmod(a, b);
    return b + fmod(a, b);
}

/* returneaza [x] (merge corect si pe numere negative) */
int int_trunc(const float x)
{
    if (x >= 0)
        return static_cast<int>(x);
    return static_cast<int>(x) - 1;
}

/* a modulo b (% nu functioneaza corect pe numere negative): rem(-1, 3) = 2 */
int rem(const int a, const int b)
{
    if (a >= 0)
        return a % b;
    return b - (-a % b);
}

/* spune daca doua numere float sunt egale */
bool floats_equal(const float x, const float y)
{
    return fabs(x - y) < EPS;
}

/* spune daca elementul (cell.row, cell.col) exista si nu e o parte dintr-o platforma */
bool nonbrick(Cell cell)
{
    if (cell.col < 0 || cell.col >= static_cast<int>(lines[0].length()))
        cell.col = rem(cell.col, lines[0].length());
    if (cell.row < 0 || cell.row >= static_cast<int>(lines.size()))
        return false;
    return lines[cell.row][cell.col] != '#';
}

/* primeste un punct si un dreptunghi cu laturile paralele cu axele si spune daca punctul se afla in
 * interiorul dreptunghiului */
bool in_rect(const Point2D &p, const Point2D &left_bottom, const Point2D &right_top)
{
    return p.x >= left_bottom.x && p.x <= right_top.x && p.y >= left_bottom.y && p.y <= right_top.y;
}

/* primeste un punct si un triunghi si spune daca punctul de afla in interiorul triunghiului */
bool in_tri(const Point2D &p, const Point2D &t1, const Point2D &t2, const Point2D &t3)
{
    vector<Point2D> triang{t1, t2, t3};
    /* caut o latura care sa separe punctul dat de celalalt varf */
    for (int i = 0; i < 3; ++i)
    {
        int j = i + 1 == 3 ? 0 : i + 1;
        int k = i + 2 >= 3 ? i - 1 : i + 2;

        if (floats_equal(triang[i].x, triang[j].x))
        {
            if ((triang[i].x - p.x) * (triang[k].x - triang[i].x) > 0)
                return false;
        }
        else
        {
            /* determin coeficientii din ecuatia explicita a dreptei (panta si intersectia cu Oy) */
            float m = (triang[i].y - triang[j].y) / (triang[i].x - triang[j].x);
            float y0 = triang[i].y - m * triang[i].x;
            if ((m * p.x + y0 - p.y) * (m * triang[k].x + y0 - triang[k].y) < 0)
                return false;
        }
    }
    return true;
}

/* primeste varfurile jucatorului si un dreptunghi cu laturile paralele cu axele si spune daca
 * jucatorul s-a lovit de acel dreptunghi si, daca da, din ce directie */
int intersect_rect(vector<Point2D> &vertices, const Point2D &left_bottom, const Point2D &right_top)
{
    /* verific daca jucatorul are un varf in dreptunghi */
    bool intersection = false;
    float xmin = player.x, ymin = player.y, xmax = player.x, ymax = player.y;
    for (int i = 0; i < 4; ++i)
    {
        if (in_rect(vertices[i], left_bottom, right_top))
            intersection = true;
        xmin = min(xmin, vertices[i].x);
        xmax = max(xmax, vertices[i].x);
        ymin = min(ymin, vertices[i].y);
        ymax = max(ymax, vertices[i].y);
    }
    /* verific daca dreptunghiul are un varf in jucator */
    float sin_u = sin(player_angle);
    float cos_u = cos(player_angle);
    if (!intersection)
    {
        float xs[] = {left_bottom.x, right_top.x};
        float ys[] = {left_bottom.y, right_top.y};
        for (int i = 0; i < 2; ++i)
        {
            for (int j = 0; j < 2; ++j)
            {
                Point2D rotated;
                rotated.x =  (xs[i] - player.x) * cos_u + (ys[j] - player.y) * sin_u;
                rotated.y = -(xs[i] - player.x) * sin_u + (ys[j] - player.y) * cos_u;
                if (in_rect(rotated, Point2D(-21, -21), Point2D(21, 21)))
                {
                    intersection = true;
                    break;
                }
            }
            if (intersection)
                break;
        }
    }
    /* luand in calcul dimensiunile elementelor din joc, conditiile de pana acum au fost
     * suficiente */
    if (!intersection)
        return NO_INT;
    /* aproximez jucatorul (posibil rotit) cu patratul minim care il contine si folosesc centrul de
     * greutate al intersectiei dintre el si dreptunghi pentru a determina directia intersectiei */
    Point2D center;
    center.x = (max(left_bottom.x, xmin) + min(right_top.x, xmax)) / 2;
    center.y = (max(left_bottom.y, ymin) + min(right_top.y, ymax)) / 2;
    if (center.x > player.x && center.y - player.y < center.x - player.x &&
        player.y - center.y < center.x - player.x)
        return INT_RIGHT;
    if (center.y > player.y)
        return INT_UP;
    return INT_DOWN;
}

/* primeste varfurile jucatorului si triunghi si spune daca jucatorul s-a lovit de acel triunghi */
bool intersect_tri(vector<Point2D> &vertices, const Point2D &t1, const Point2D &t2,
                   const Point2D &t3)
{
    /* verific daca jucatorul are un varf in triunghi */
    for (int i = 0; i < 4; ++i)
        if (in_tri(vertices[i], t1, t2, t3))
            return true;

    /* verific daca triunghiul are un varf in jucator */
    float sin_u = sin(player_angle);
    float cos_u = cos(player_angle);

    Point2D t1_rotated, t2_rotated, t3_rotated;
    t1_rotated.x =  (t1.x - player.x) * cos_u + (t1.y - player.y) * sin_u;
    t1_rotated.y = -(t1.x - player.x) * sin_u + (t1.y - player.y) * cos_u;
    if (in_rect(t1_rotated, Point2D(-21, -21), Point2D(21, 21)))
        return true;
    t2_rotated.x =  (t2.x - player.x) * cos_u + (t2.y - player.y) * sin_u;
    t2_rotated.y = -(t2.x - player.x) * sin_u + (t2.y - player.y) * cos_u;
    if (in_rect(t2_rotated, Point2D(-21, -21), Point2D(21, 21)))
        return true;
    t3_rotated.x =  (t3.x - player.x) * cos_u + (t3.y - player.y) * sin_u;
    t3_rotated.y = -(t3.x - player.x) * sin_u + (t3.y - player.y) * cos_u;
    if (in_rect(t3_rotated, Point2D(-21, -21), Point2D(21, 21)))
        return true;

    /* luand in calcul dimensiunile elementelor din joc, conditiile de pana acum au fost
     * suficiente */
    return false;
}

/* spune daca steguletul dintr-o celula data a fost colectat */
bool collected_flag(const Cell &cell)
{
    return find(collected.begin(), collected.end(), cell) != collected.end();
}

/* adauga un cerc (folosit la atingerea anumitor elemente) */
void add_effect_circle(const Point2D &center)
{
    Circle2D *circle = new Circle2D(center, 1, Color(1, 1, 1), false);
    DrawingWindow::objects2D.insert(DrawingWindow::objects2D.begin() + 1 + last_circle_pos, circle);
    ++last_circle_pos;
    ++first_traj_pos;
    ++last_traj_pos;
    ++first_elem_pos;
    ++last_elem_pos;
    circles.push_back(circle);
    circle_positions.push_back(center);
    circles_time.push_back(idle_calls);
}

/* muta obiectele ce alcatuiesc pamantul in fereastra din coordonate logice */
void ground_to_camera()
{
    ground_offset = camera.x;
    first_tile = 0;
    for (int i = 0; i < 5; ++i) /* "coloanele" verticale din pattern-ul pamantului */
        for (int j = 0; j < 6; ++j) /* obiectul din cadrul "coloanei" */
        {
            Transform2D::loadIdentityMatrix();
            Transform2D::translateMatrix(ground_offset + i * 200, 0);
            Transform2D::applyTransform(ground[i * 6 + j]);
        }
}

/* actualizeaza panourile din fundal, incat sa umple tot spatiul vizibil */
void update_background()
{
    Point2D backgr_origin;
    backgr_origin.x = BACK_RATIO * camera.x + ground_offset * (1 - BACK_RATIO);
    backgr_origin.y = BACK_RATIO * camera.y - 127 * (1 - BACK_RATIO);
    Point2D center(camera.x + 400, camera.y + 225);

    /* cu cat se translateaza panoul din fundal care contine centrul camerei */
    Point2D first_panel;
    first_panel.x = center.x - real_modulo(center.x - backgr_origin.x, 850);
    first_panel.y = center.y - real_modulo(center.y - backgr_origin.y, 850);

    /* pe orizontala, se vor afisa panouri la stanga sau la dreapta celui ce contine centrul
     * camerei? */
    bool left  = real_modulo(center.x - backgr_origin.x, 850) < 425;
    /* pe verticala, se vor afisa panouri deasupra sau dedesubtul celui ce contine centrul
     * camerei? */
    bool below = real_modulo(center.y - backgr_origin.y, 850) < 425;

    for (int row = 0; row < 2; ++row)
        for (int col = 0; col < 2; ++col)
            for (unsigned int i = 0; i < background[row * 2 + col].size(); ++i)
            {
                Transform2D::loadIdentityMatrix();
                Transform2D::translateMatrix(first_panel.x + (col == 1 ? (left ? -850 : 850) : 0),
                                             first_panel.y + (row == 1 ? (below ? -850 : 850) : 0));
                Transform2D::applyTransform(background[row * 2 + col][i]);
            }
}

/* actualizeaza obiectele vizibile */
void draw_visible_objects()
{
    /* sterg toate obiectele desenate momentan */
    if (last_elem_pos >= first_elem_pos)
    {
        DrawingWindow::objects2D.erase(DrawingWindow::objects2D.begin() + first_elem_pos,
                                       DrawingWindow::objects2D.begin() + 1 + last_elem_pos);
        last_elem_pos = first_elem_pos - 1;
    }
    auto_jump_list.clear();

    first_seen_col = int_trunc(camera.x / 42) + (real_modulo(camera.x, 42) >= 21 ? 1 : 0);
    last_seen_col = int_trunc((camera.x + 800) / 42) +
                    (real_modulo(camera.x + 800, 42) >= 21 ? 0 : -1);
    for (int j = first_seen_col; j <= last_seen_col; ++j)
    {
        int col = rem(j, objects[0].size());
        for (unsigned int i = 0; i < objects.size(); ++i)
            if (!collected_flag(Cell(i, col)))
            {
                Transform2D::loadIdentityMatrix();
                Transform2D::translateMatrix(-21, -21);
                Transform2D::scaleMatrix(1, 1);
                Transform2D::translateMatrix(42.0f * j + 21.0f,
                                             42.0f * (objects.size() - 1 - i) + 21.0f);
                for (unsigned int elem = 0; elem < objects[i][col].size(); ++elem)
                    Transform2D::applyTransform(objects[i][col][elem]);
                DrawingWindow::objects2D.insert(
                    DrawingWindow::objects2D.begin() + 1 + last_elem_pos,
                    objects[i][col].begin(), objects[i][col].end());
                if ((lines[i][col] == '.') && !(i + 1 >= lines.size() || lines[i + 1][col] == '#' ||
                    lines[i + 1][col] == '-' || lines[i + 1][col] == 'n' ||
                    lines[i + 1][col] == 'V' || lines[i + 1][col] == 'v'))
                    auto_jump_list.push_back(Cell(i, j));
                last_elem_pos += objects[i][col].size();
            }
    }
}

/* procedura executata cand jucatorul moare */
void die()
{
    dead = true;
    death_time = idle_calls;
    if (lifes > 0)
    {
        --lifes;
        Text *new_lifes = new Text("Incercari: " + to_string(lifes),
                                   Point2D(game_width - 92.0f, -1037), Color(1, 1, 1),
                                   BITMAP_HELVETICA_18);
        DrawingWindow::texts[1] = new_lifes;
    }

    vector<Object2D *>::iterator it = find(DrawingWindow::objects2D.begin(),
                                           DrawingWindow::objects2D.end(),
                                           player_graphics[0]);
    DrawingWindow::objects2D.erase(it, it + player_graphics.size());

    first_explosion_pos -= player_graphics.size();
    last_explosion_pos -= player_graphics.size();
    first_circle_pos -= player_graphics.size();
    last_circle_pos -= player_graphics.size();
    first_traj_pos -= player_graphics.size();
    last_traj_pos -= player_graphics.size();
    first_elem_pos -= player_graphics.size();
    last_elem_pos -= player_graphics.size();

    vector<Object2D *> lines;
    for (int i = 0; i < EXP_LINES; ++i)
    {
        float angle = 2 * PI * rand() / RAND_MAX;
        float distance = 20.0f + 33.0f * rand() / RAND_MAX;
        explosion_angles.push_back(angle);
        Line2D *line = new Line2D(Point2D(player.x + cos(angle) * distance,
                                          player.y + sin(angle) * distance),
                                  Point2D(player.x + cos(angle) * (distance + EXP_LEN),
                                          player.y + sin(angle) * (distance + EXP_LEN)),
                                  Color(1.0f, 0.06f + 0.93f * rand() / RAND_MAX, 0.06f));
        lines.push_back(line);
    }

    DrawingWindow::objects2D.insert(DrawingWindow::objects2D.begin() + 1 + last_explosion_pos,
                                    lines.begin(), lines.end());

    last_explosion_pos += EXP_LINES;
    first_circle_pos += EXP_LINES;
    last_circle_pos += EXP_LINES;
    first_traj_pos += EXP_LINES;
    last_traj_pos += EXP_LINES;
    first_elem_pos += EXP_LINES;
    last_elem_pos += EXP_LINES;

    if (collected.size() && lifes == 0)
    {
        /* game over */
        waiting_for_key = true;
        Text *lost = new Text("Ai pierdut.", Point2D((game_width - 88.0f) / 2 - 1, -1037),
                              Color(1.0f, 0.5f, 0.5f), BITMAP_HELVETICA_18);
        DrawingWindow::texts[0] = lost;
        game_over = true;
    }
}

/* inceperea unui joc nou */
void start()
{
    /* reinitializari */
    if (!game_won)
    {
        DrawingWindow::objects2D.insert(DrawingWindow::objects2D.begin(), player_graphics.begin(),
                                        player_graphics.end());
        first_explosion_pos += player_graphics.size();
        last_explosion_pos += player_graphics.size();
        first_circle_pos += player_graphics.size();
        last_circle_pos += player_graphics.size();
        first_traj_pos += player_graphics.size();
        last_traj_pos += player_graphics.size();
        first_elem_pos += player_graphics.size();
        last_elem_pos += player_graphics.size();
    }
    else
        lifes = 0;

    if (collected.size() && lifes == 0)
    {
        collected.clear();
        Text *new_score = new Text("Puncte: 0 / " + to_string(FLAGS_NEEDED),
                                   Point2D((game_width - 101.0f) / 2 - 1, -1037), Color(1, 1, 1),
                                   BITMAP_HELVETICA_18);
        DrawingWindow::texts[0] = new_score;
        Text *new_lifes = new Text(" ", Point2D(0, 0), Color(1, 1, 1), BITMAP_HELVETICA_18);
        DrawingWindow::texts[1] = new_lifes;
    }
    if (!collected.size())
    {
        player.x = 42.0f * player_cell.col + 21.0f;
        player.y = 42.0f * (lines.size() - 1 - player_cell.row) + 21.0f;
        player_line = player_cell.row + 1;
        camera.x = player.x - 63;
        follow_player = false;
    }
    else
    {
        player.x = 42.0f * last_checkpoint.col + 21.0f;
        player.y = 42.0f * (lines.size() - 1 - last_checkpoint.row) + 21.0f;
        player_line = last_checkpoint.row + 1;
        camera.x = player.x - 6.5f * 42;
        follow_player = true;
    }

    camera.y = player.y - 148;
    player_angle = 0;
    jump_start_y = 0;
    ground_to_camera();
    draw_visible_objects();

    vector<Object2D *> traj_to_delete(DrawingWindow::objects2D.begin() + first_traj_pos,
                                      DrawingWindow::objects2D.begin() + 1 + last_traj_pos);
    DrawingWindow::objects2D.erase(DrawingWindow::objects2D.begin() + first_traj_pos,
                                   DrawingWindow::objects2D.begin() + 1 + last_traj_pos);
    int lines_num = last_traj_pos - first_traj_pos + 1;
    last_traj_pos = first_traj_pos - 1;
    first_elem_pos -= lines_num;
    last_elem_pos -= lines_num;
    for (unsigned int i = 0; i < traj_to_delete.size(); ++i)
        delete traj_to_delete[i];
    vector<Object2D *> circles_to_delete(DrawingWindow::objects2D.begin() + first_circle_pos,
                                         DrawingWindow::objects2D.begin() + 1 + last_circle_pos);
    DrawingWindow::objects2D.erase(DrawingWindow::objects2D.begin() + first_circle_pos,
                                   DrawingWindow::objects2D.begin() + 1 + last_circle_pos);

    last_circle_pos -= circles.size();
    first_traj_pos  -= circles.size();
    last_traj_pos   -= circles.size();
    first_elem_pos  -= circles.size();
    last_elem_pos   -= circles.size();

    circles.clear();
    circle_positions.clear();
    circles_time.clear();
    for (unsigned int i = 0; i < circles_to_delete.size(); ++i)
        delete circles_to_delete[i];
    vector<Object2D *> lines_to_delete(DrawingWindow::objects2D.begin() + first_explosion_pos,
                                       DrawingWindow::objects2D.begin() + 1 + last_explosion_pos);
    DrawingWindow::objects2D.erase(DrawingWindow::objects2D.begin() + first_explosion_pos,
                                   DrawingWindow::objects2D.begin() + 1 + last_explosion_pos);

    last_explosion_pos -= lines_to_delete.size();
    first_circle_pos   -= lines_to_delete.size();
    last_circle_pos    -= lines_to_delete.size();
    first_traj_pos     -= lines_to_delete.size();
    last_traj_pos      -= lines_to_delete.size();
    first_elem_pos     -= lines_to_delete.size();
    last_elem_pos      -= lines_to_delete.size();

    explosion_angles.clear();
    for (unsigned int i = 0; i < lines_to_delete.size(); ++i)
        delete lines_to_delete[i];

    state = SEATED;
    elevated = false;
    auto_jump = false;
    jump_elem = Cell(-1, -1);
    update_background();
    dead = false;
    waiting_for_key = false;
    game_over = false;
    game_won = false;
    won_column = 0;
}

/* functia care permite adaugarea de obiecte */
void DrawingWindow::init()
{
    srand(static_cast<unsigned int>(time(NULL)));

    /* initializare culori */
    colors = {
        new Color(0.57f, 1.00f, 0.87f),
        new Color(0.10f, 0.33f, 0.67f),
        new Color(0.09f, 0.24f, 0.51f),
        new Color(0.05f, 0.16f, 0.35f),
        new Color(0.02f, 0.08f, 0.14f),
        new Color(0.15f, 0.38f, 0.75f),
        new Color(0.00f, 0.40f, 1.00f),
        new Color(0.14f, 0.46f, 0.96f)};

    /* initializare ultimul element de jump */
    jump_elem = Cell(-1, -1);

    /* citire, retinere coordonate jucator si verificare corectitudine */
    ifstream map_file("map.txt");
    if (!map_file.is_open())
    {
        cerr << "Nu s-a gasit fisierul cu harta (map.txt).\n";
        exit(1);
    }
    string line;
    int line_number = 0, flags_found = 0;
    player_cell.row = -1;
    while (getline(map_file, line))
    {
        if (lines.size() > 0 && line.length() != lines[0].length())
        {
            cerr << "map.txt, linia " << line_number + 1 <<
                ": Fisierul cu harta trebuie sa aiba toate liniile de aceeasi lungime.\n";
            exit(1);
        }
        if (line.length() < 20)
        {
            cerr << "map.txt, linia " << line_number + 1 <<
                ": Fisierul cu harta nu poate avea linii mai scurte de 20 de caractere.\n";
            exit(1);
        }
        int S_pos = line.find('S');
        if (S_pos != string::npos)
            player_cell = Cell(line_number, S_pos);
        flags_found += count(line.begin(), line.end(), 'P');
        lines.push_back(line);
        ++line_number;
    }
    map_file.close();
    if (!line_number)
    {
        cerr << "Fisierul de intrare trebuie sa contina cel putin o linie.\n";
        exit(1);
    }
    if (player_cell.row == -1)
    {
        cerr << "In fisierul de intrare nu se gaseste pozitia de start ('S').\n";
        exit(1);
    }
    if (flags_found < FLAGS_NEEDED)
        cerr << "In fisierul de intrare nu exista minimul de " << FLAGS_NEEDED <<
            " stegulete pentru ca jocul sa poata fi castigat.\n";

    /* construire jucator (inainte de scena, pentru ca el sa apara in fata) */
    Transform2D::loadIdentityMatrix();
    Transform2D::translateMatrix(42.0f * player_cell.col + 21.0f,
                                 42.0f * (lines.size() - 1 - player_cell.row) + 21.0f);
    Polygon2D* border_right_eye = new Polygon2D(Color(0, 0, 0), false);
    border_right_eye->addPoint(Point2D(-17, 5));
    border_right_eye->addPoint(Point2D(-17, 14));
    border_right_eye->addPoint(Point2D(-6, 8));
    border_right_eye->addPoint(Point2D(-6, 2));
    Transform2D::applyTransform(border_right_eye);
    player_graphics.push_back(border_right_eye);
    Polygon2D* border_left_eye = new Polygon2D(Color(0, 0, 0), false);
    border_left_eye->addPoint(Point2D(17, 5));
    border_left_eye->addPoint(Point2D(17, 14));
    border_left_eye->addPoint(Point2D(6, 8));
    border_left_eye->addPoint(Point2D(6, 2));
    Transform2D::applyTransform(border_left_eye);
    player_graphics.push_back(border_left_eye);
    Polygon2D* border_mouth = new Polygon2D(Color(0, 0, 0), false);
    border_mouth->addPoint(Point2D(-17, 1));
    border_mouth->addPoint(Point2D(0, -3));
    border_mouth->addPoint(Point2D(17, 1));
    border_mouth->addPoint(Point2D(17, -8));
    border_mouth->addPoint(Point2D(0, -18));
    border_mouth->addPoint(Point2D(-17, -8));
    Transform2D::applyTransform(border_mouth);
    player_graphics.push_back(border_mouth);
    Rectangle2D *border_face = new Rectangle2D(Point2D(-21, -21), 42, 42, Color(0, 0, 0), false);
    Transform2D::applyTransform(border_face);
    player_graphics.push_back(border_face);

    Polygon2D* right_eye = new Polygon2D(Color(0.3f, 1, 1), true);
    right_eye->addPoint(Point2D(-17, 5));
    right_eye->addPoint(Point2D(-17, 14));
    right_eye->addPoint(Point2D(-6, 8));
    right_eye->addPoint(Point2D(-6, 2));
    Transform2D::applyTransform(right_eye);
    player_graphics.push_back(right_eye);
    Polygon2D* left_eye = new Polygon2D(Color(0.3f, 1, 1), true);
    left_eye->addPoint(Point2D(17, 5));
    left_eye->addPoint(Point2D(17, 14));
    left_eye->addPoint(Point2D(6, 8));
    left_eye->addPoint(Point2D(6, 2));
    Transform2D::applyTransform(left_eye);
    player_graphics.push_back(left_eye);
    Polygon2D* mouth1 = new Polygon2D(Color(0.3f, 1, 1), true);
    mouth1->addPoint(Point2D(-17, 1));
    mouth1->addPoint(Point2D(0, -3));
    mouth1->addPoint(Point2D(0, -18));
    mouth1->addPoint(Point2D(-17, -8));
    Transform2D::applyTransform(mouth1);
    player_graphics.push_back(mouth1);
    Polygon2D* mouth2 = new Polygon2D(Color(0.3f, 1, 1), true);
    mouth2->addPoint(Point2D(0, -3));
    mouth2->addPoint(Point2D(17, 1));
    mouth2->addPoint(Point2D(17, -8));
    mouth2->addPoint(Point2D(0, -18));
    Transform2D::applyTransform(mouth2);
    player_graphics.push_back(mouth2);
    Rectangle2D *face = new Rectangle2D(Point2D(-21.0f, -21.0f), 42, 42, Color(0, 0.7f, 0), true);
    Transform2D::applyTransform(face);
    player_graphics.push_back(face);
    for (unsigned int elem = 0; elem < player_graphics.size(); ++elem)
        addObject2D(player_graphics[elem]);
    state = SEATED;
    auto_jump = false;
    player_line = player_cell.row + 1;
    elevated = false;

    /* retin aceasta pozitie, unde vor fi inserate obiecte pentru simularea anumitor efecte */
    first_traj_pos = objects2D.size();
    last_traj_pos = first_traj_pos - 1;
    first_circle_pos = objects2D.size();
    last_circle_pos = first_circle_pos - 1;
    first_explosion_pos = objects2D.size();
    last_explosion_pos = first_explosion_pos - 1;

    /* creez contextele vizuale pentru scor, incercari si fundal */
    Rectangle2D *window_backgr = new Rectangle2D(Point2D(-4, -1014), 18, 18, Color(0, 0, 0), true);
    addObject2D(window_backgr);
    script_rect = new Rectangle2D(Point2D(16, -1014), 18, 18, *colors[2], true);
    addObject2D(script_rect);
    script = new Visual2D(0, -1040, 800, -1020, 0, 0, DrawingWindow::width, 20);
    addVisual2D(script);
    script_backgr = new Visual2D(20, -1010, 30, -1000, 0, 0, DrawingWindow::width, 20);
    addVisual2D(script_backgr);
    Text *score = new Text("Puncte: 0 / " + to_string(FLAGS_NEEDED), Point2D(349, -1037),
                           Color(1, 1, 1), BITMAP_HELVETICA_18);
    addText(score);
    /* pentru vieti */
    addText(new Text(" ", Point2D(0, 0), Color(1, 1, 1), BITMAP_HELVETICA_18));
    /* pentru textul "Pauza" */
    addText(new Text(" ", Point2D(0, 0), Color(1, 1, 1), BITMAP_HELVETICA_18));
    game_width = 800;

    /* creez un context vizual si pozitionez "camera" in functie de pozitia jucatorului */
    player.x = 42.0f * player_cell.col + 21.0f;
    player.y = 42.0f * (lines.size() - 1 - player_cell.row) + 21.0f;
    camera.y = player.y - 148;
    camera.x = player.x - 63;
    game = new Visual2D(camera.x, camera.y, camera.x + DrawingWindow::width,
                        camera.y + DrawingWindow::height, 0, 21, DrawingWindow::width,
                        DrawingWindow::height);
    addVisual2D(game);
    backgr1 = new Visual2D(0, -1010, 10, -1000, 0, 0, 0, DrawingWindow::height);
    addVisual2D(backgr1);
    backgr2 = new Visual2D(0, -1010, 10, -1000, DrawingWindow::width, 0, DrawingWindow::width,
                           DrawingWindow::height);
    addVisual2D(backgr2);

    /* creez pamantul si tavanul */
    for (int i = 0; i < 5; ++i)
    {
        Line2D *lower_border = new Line2D(Point2D(-5, 0), Point2D(205, 0), Color(1, 1, 1));
        ground.push_back(lower_border);
        addObject2D(lower_border);
        Line2D *upper_border = new Line2D(Point2D(-5, lines.size() * 42.0f),
                                          Point2D(205, lines.size() * 42.0f), Color(1, 1, 1));
        ground.push_back(upper_border);
        addObject2D(upper_border);
        Rectangle2D *dark_lower_tile = new Rectangle2D(Point2D(11, -150), 178, 142, *colors[1],
                                                       true);
        ground.push_back(dark_lower_tile);
        addObject2D(dark_lower_tile);
        Rectangle2D *bright_lower_tile = new Rectangle2D(Point2D(-5, -150), 210, 150, *colors[6],
                                                         true);
        ground.push_back(bright_lower_tile);
        addObject2D(bright_lower_tile);
        Rectangle2D *dark_upper_tile = new Rectangle2D(Point2D(11, lines.size() * 42.0f + 8),
                                                       178, 292, *colors[1], true);
        ground.push_back(dark_upper_tile);
        addObject2D(dark_upper_tile);
        Rectangle2D *bright_upper_tile = new Rectangle2D(Point2D(-5, lines.size() * 42.0f),
                                                         210, 300, *colors[6], true);
        ground.push_back(bright_upper_tile);
        addObject2D(bright_upper_tile);
    }
    ground_to_camera();

    /* marchez actuala pozitie in vectorul de obiecte 2D, pentru a insera aici ulterior alte
     * elemente care sa apara mai in spate */
    first_elem_pos = objects2D.size();
    last_elem_pos = first_elem_pos - 1;

    /* construire obiecte */
    objects.resize(lines.size());
    for (unsigned int i = 0; i < objects.size(); ++i)
        objects[i].resize(lines[0].length());
    for (int j = 0; j < static_cast<int>(lines[0].length()); ++j)
        for (int i = 0; i < static_cast<int>(lines.size()); ++i)
        {
            Transform2D::loadIdentityMatrix();
            Transform2D::translateMatrix(42.0f * j, 42.0f * (lines.size() - 1 - i));
            switch (lines[i][j])
            {
                case '#': /* o "caramida" dintr-o platforma */
                {
                    Line2D *line1 = new Line2D(Point2D(14, 0), Point2D(14, 42), *colors[5]);
                    Transform2D::applyTransform(line1);
                    objects[i][j].push_back(line1);
                    Line2D *line2 = new Line2D(Point2D(28, 0), Point2D(28, 42), *colors[5]);
                    Transform2D::applyTransform(line2);
                    objects[i][j].push_back(line2);
                    Line2D *line3 = new Line2D(Point2D(0, 14), Point2D(42, 14), *colors[5]);
                    Transform2D::applyTransform(line3);
                    objects[i][j].push_back(line3);
                    Line2D *line4 = new Line2D(Point2D(0, 28), Point2D(42, 28), *colors[5]);
                    Transform2D::applyTransform(line4);
                    objects[i][j].push_back(line4);
                    for (int p = -1; p < 2; ++p)
                        for (int q = p ? 0 : -1; q < 2; q += 2)
                            if (nonbrick(Cell(i + p, j + q)))
                            {
                                Line2D *border = new Line2D(
                                    p == -1 || q == -1 ? Point2D(0, 42) : Point2D(42, 0),
                                    p == -1 || q == 1 ? Point2D(42, 42) : Point2D(0, 0),
                                    Color(1, 1, 1));
                                Transform2D::applyTransform(border);
                                objects[i][j].push_back(border);
                            }
                    bool found_nonbrick = false;
                    for (int p = -1; p < 2; ++p)
                        for (int q = -1; q < 2; ++q)
                            if (p || q)
                            {
                                /* pentru culoarea centrului */
                                if (nonbrick(Cell(i + p, j + q)))
                                    found_nonbrick = true;

                                int type = 0; /* va putea fi 1, 2, 3 sau 4 */
                                /* daca e un patratel din colt */
                                if (p && q)
                                    if (nonbrick(Cell(i + p, j + q)) || nonbrick(Cell(i + p, j)) ||
                                        nonbrick(Cell(i, j + q)))
                                        type = 4;
                                    else
                                        type = 2;
                                else /* e un patratel de pe latura */
                                    if (nonbrick(Cell(i + p, j + q)))
                                        type = 4;
                                    else if (!p)
                                        type = nonbrick(Cell(i - 1, j + q)) ||
                                               nonbrick(Cell(i + 1, j + q)) ||
                                               nonbrick(Cell(i - 1, j)) ||
                                               nonbrick(Cell(i + 1, j)) ? 3 : 2;
                                    else
                                        type = nonbrick(Cell(i + p, j - 1)) ||
                                               nonbrick(Cell(i + p, j + 1)) ||
                                               nonbrick(Cell(i, j - 1)) ||
                                               nonbrick(Cell(i, j + 1)) ? 3 : 2;
                                Rectangle2D *ninth = new Rectangle2D(
                                    Point2D((q + 1) * 14.0f, (1 - p) * 14.0f), 14, 14,
                                    *colors[type], true);
                                Transform2D::applyTransform(ninth);
                                objects[i][j].push_back(ninth);
                            }
                    Rectangle2D *center = new Rectangle2D(Point2D(14, 14), 14, 14,
                                                          *colors[found_nonbrick ? 3 : 1], true);
                    Transform2D::applyTransform(center);
                    objects[i][j].push_back(center);
                }
                break;
                case '_': /* dreptunghi izolat, in partea de jos a spatiului patratic */
                {
                    Rectangle2D *border = new Rectangle2D(Point2D(0, 0), 42, 17, Color(1, 1, 1),
                                                          false);
                    Transform2D::applyTransform(border);
                    objects[i][j].push_back(border);
                    Rectangle2D *rect = new Rectangle2D(Point2D(0, 0), 42, 17, *colors[4],
                                                        true);
                    Transform2D::applyTransform(rect);
                    objects[i][j].push_back(rect);
                }
                break;
                case '-': /* dreptunghi izolat, in partea de sus a spatiului patratic */
                {
                    Rectangle2D *border = new Rectangle2D(Point2D(0, 25), 42, 17, Color(1, 1, 1),
                                                          false);
                    Transform2D::applyTransform(border);
                    objects[i][j].push_back(border);
                    Rectangle2D *rect = new Rectangle2D(Point2D(0, 25), 42, 17, *colors[4],
                                                        true);
                    Transform2D::applyTransform(rect);
                    objects[i][j].push_back(rect);
                }
                break;
                case 'n': /* un patrat */
                {
                    Rectangle2D *border = new Rectangle2D(Point2D(0, 0), 42, 42, Color(1, 1, 1),
                                                          false);
                    Transform2D::applyTransform(border);
                    objects[i][j].push_back(border);
                    Rectangle2D *rect = new Rectangle2D(Point2D(0, 0), 42, 42, *colors[4],
                                                        true);
                    Transform2D::applyTransform(rect);
                    objects[i][j].push_back(rect);
                }
                break;
                case '.': /* cerc pentru jump automat (va fi pozitionat pe latura de jos a
                           * patratului) */
                    if (i + 1 >= static_cast<int>(lines.size()) || lines[i + 1][j] == '#' ||
                                                  lines[i + 1][j] == '-' ||
                                                  lines[i + 1][j] == 'n' ||
                                                  lines[i + 1][j] == 'V' ||
                                                  lines[i + 1][j] == 'v')
                    {
                        Polygon2D* border = new Polygon2D(Color(1, 1, 1), false);
                        border->addPoint(Point2D( 0, 0));
                        border->addPoint(Point2D( 4, 4));
                        border->addPoint(Point2D(11, 7));
                        border->addPoint(Point2D(21, 9));
                        border->addPoint(Point2D(31, 7));
                        border->addPoint(Point2D(38, 4));
                        border->addPoint(Point2D(42, 0));
                        Transform2D::applyTransform(border);
                        objects[i][j].push_back(border);
                        Polygon2D* polygon = new Polygon2D(Color(1, 0.98f, 0.38f), true);
                        polygon->addPoint(Point2D( 0, 0));
                        polygon->addPoint(Point2D( 4, 4));
                        polygon->addPoint(Point2D(11, 7));
                        polygon->addPoint(Point2D(21, 9));
                        polygon->addPoint(Point2D(31, 7));
                        polygon->addPoint(Point2D(38, 4));
                        polygon->addPoint(Point2D(42, 0));
                        Transform2D::applyTransform(polygon);
                        objects[i][j].push_back(polygon);
                    }
                    else
                    {
                        Circle2D *border = new Circle2D(Point2D(21, 0), 19, Color(1, 1, 1), false);
                        Transform2D::applyTransform(border);
                        objects[i][j].push_back(border);
                        Circle2D *dot = new Circle2D(Point2D(21, 0), 14, Color(1, 0.98f, 0.38f),
                                                     true);
                        Transform2D::applyTransform(dot);
                        objects[i][j].push_back(dot);
                    }
                break;
                case 'A': /* triunghi mare cu varful in sus */
                case 'a': /* triunghi mic cu varful in sus */
                {
                    Polygon2D *border = new Polygon2D(Color(1, 1, 1), false);
                    border->addPoint(Point2D(0, 0));
                    border->addPoint(Point2D(21, lines[i][j] == 'A' ? 42.0f : 15.0f));
                    border->addPoint(Point2D(42, 0));
                    Transform2D::applyTransform(border);
                    objects[i][j].push_back(border);
                    Polygon2D *triangle = new Polygon2D(*colors[4], true);
                    triangle->addPoint(Point2D(0, 0));
                    triangle->addPoint(Point2D(21, lines[i][j] == 'A' ? 42.0f : 15.0f));
                    triangle->addPoint(Point2D(42, 0));
                    Transform2D::applyTransform(triangle);
                    objects[i][j].push_back(triangle);
                }
                break;
                case 'V': /* triunghi mare cu varful in jos */
                case 'v': /* triunghi mic cu varful in jos */
                {
                    Polygon2D *border = new Polygon2D(Color(1, 1, 1), false);
                    border->addPoint(Point2D(0, 42));
                    border->addPoint(Point2D(21, lines[i][j] == 'V' ? 0.0f : 27.0f));
                    border->addPoint(Point2D(42, 42));
                    Transform2D::applyTransform(border);
                    objects[i][j].push_back(border);
                    Polygon2D *triangle = new Polygon2D(*colors[4], true);
                    triangle->addPoint(Point2D(0, 42));
                    triangle->addPoint(Point2D(21, lines[i][j] == 'V' ? 0.0f : 27.0f));
                    triangle->addPoint(Point2D(42, 42));
                    Transform2D::applyTransform(triangle);
                    objects[i][j].push_back(triangle);
                }
                break;
                case '>': /* triunghi mare cu varful spre dreapta */
                case ';': /* triunghi mic cu varful spre dreapta */
                {
                    Polygon2D *border = new Polygon2D(Color(1, 1, 1), false);
                    border->addPoint(Point2D(0, 42));
                    border->addPoint(Point2D(lines[i][j] == '>' ? 42.0f : 15.0f, 21));
                    border->addPoint(Point2D(0, 0));
                    Transform2D::applyTransform(border);
                    objects[i][j].push_back(border);
                    Polygon2D *triangle = new Polygon2D(*colors[4], true);
                    triangle->addPoint(Point2D(0, 42));
                    triangle->addPoint(Point2D(lines[i][j] == '>' ? 42.0f : 15.0f, 21));
                    triangle->addPoint(Point2D(0, 0));
                    Transform2D::applyTransform(triangle);
                    objects[i][j].push_back(triangle);
                }
                break;
                case '<': /* triunghi mare cu varful spre stanga */
                case ':': /* triunghi mic cu varful spre stanga */
                {
                    Polygon2D *border = new Polygon2D(Color(1, 1, 1), false);
                    border->addPoint(Point2D(lines[i][j] == '<' ? 0.0f : 27.0f, 21));
                    border->addPoint(Point2D(42, 42));
                    border->addPoint(Point2D(42, 0));
                    Transform2D::applyTransform(border);
                    objects[i][j].push_back(border);
                    Polygon2D *triangle = new Polygon2D(*colors[4], true);
                    triangle->addPoint(Point2D(lines[i][j] == '<' ? 0.0f : 27.0f, 21));
                    triangle->addPoint(Point2D(42, 42));
                    triangle->addPoint(Point2D(42, 0));
                    Transform2D::applyTransform(triangle);
                    objects[i][j].push_back(triangle);
                }
                break;
                case '^': /* spatiu dintre platforme */
                {
                    Polygon2D *triang1 = new Polygon2D(Color(0, 0, 0), true);
                    triang1->addPoint(Point2D(0,  0));
                    triang1->addPoint(Point2D(0,  2));
                    triang1->addPoint(Point2D(4, 11));
                    triang1->addPoint(Point2D(7,  5));
                    triang1->addPoint(Point2D(7,  0));
                    Transform2D::applyTransform(triang1);
                    objects[i][j].push_back(triang1);

                    Polygon2D *triang2 = new Polygon2D(Color(0, 0, 0), true);
                    triang2->addPoint(Point2D( 7,  0));
                    triang2->addPoint(Point2D( 7,  5));
                    triang2->addPoint(Point2D(11, 13));
                    triang2->addPoint(Point2D(15,  5));
                    triang2->addPoint(Point2D(15,  0));
                    Transform2D::applyTransform(triang2);
                    objects[i][j].push_back(triang2);

                    Polygon2D *triang3 = new Polygon2D(Color(0, 0, 0), true);
                    triang3->addPoint(Point2D(15,  0));
                    triang3->addPoint(Point2D(15,  5));
                    triang3->addPoint(Point2D(20, 16));
                    triang3->addPoint(Point2D(25,  5));
                    triang3->addPoint(Point2D(25,  0));
                    Transform2D::applyTransform(triang3);
                    objects[i][j].push_back(triang3);

                    Polygon2D *triang4 = new Polygon2D(Color(0, 0, 0), true);
                    triang4->addPoint(Point2D(25,  0));
                    triang4->addPoint(Point2D(25,  5));
                    triang4->addPoint(Point2D(29, 12));
                    triang4->addPoint(Point2D(33,  4));
                    triang4->addPoint(Point2D(33,  0));
                    Transform2D::applyTransform(triang4);
                    objects[i][j].push_back(triang4);

                    Polygon2D *triang5 = new Polygon2D(Color(0, 0, 0), true);
                    triang5->addPoint(Point2D(33,  0));
                    triang5->addPoint(Point2D(33,  4));
                    triang5->addPoint(Point2D(37, 12));
                    triang5->addPoint(Point2D(42,  2));
                    triang5->addPoint(Point2D(42,  0));
                    Transform2D::applyTransform(triang5);
                    objects[i][j].push_back(triang5);

                    Polygon2D *triang6 = new Polygon2D(*colors[0], true);
                    triang6->addPoint(Point2D(21,  0));
                    triang6->addPoint(Point2D(30, 19));
                    triang6->addPoint(Point2D(39,  0));
                    Transform2D::applyTransform(triang6);
                    objects[i][j].push_back(triang6);
                }
                break;
                case 'P': /* stegulet */
                {
                    Polygon2D *border = new Polygon2D(Color(0, 0, 0), false);
                    border->addPoint(Point2D( 3, 32));
                    border->addPoint(Point2D(21, 42));
                    border->addPoint(Point2D(21, 22));
                    Transform2D::applyTransform(border);
                    objects[i][j].push_back(border);
                    Polygon2D *canvas = new Polygon2D(Color(1, 1, 1), true);
                    canvas->addPoint(Point2D( 3, 32));
                    canvas->addPoint(Point2D(21, 42));
                    canvas->addPoint(Point2D(21, 22));
                    Transform2D::applyTransform(canvas);
                    objects[i][j].push_back(canvas);
                    Rectangle2D *stick = new Rectangle2D(Point2D(20, 0), 3, 42, Color(0, 0, 0),
                                                         true);
                    Transform2D::applyTransform(stick);
                    objects[i][j].push_back(stick);
                }
            }
        }

    /* crearea matricei care se deseneaza cand jocul e castigat */
    won_matr_obj.resize(4);
    for (int i = 0; i < 4; ++i)
    {
        won_matr_obj[i].resize(30);
        for (int j = 0; j < 30; ++j)
            if (won_matr_char[i][j])
            {
                Rectangle2D *border = new Rectangle2D(Point2D(0, 0), 42, 42, Color(1, 1, 1), false);
                won_matr_obj[i][j].push_back(border);
                Rectangle2D *rect = new Rectangle2D(Point2D(0, 0), 42, 42, *colors[4], true);
                won_matr_obj[i][j].push_back(rect);
            }
    }

    /* creez fundalul din 4 parti identice, care se vor translata acolo unde e camera */
    background.resize(4);
    for (int i = 0; i < 4; ++i)
    {
        int corner_x[] = {  1, 211, 368, 575,   0, 108, 211, 369, 573, 573, 640,   1,   2,   2,  66,   2, 158, 163, 163, 370, 370, 642, 780, 780, 647};
        int corner_y[] = {  0,   0,   0,   0, 209, 209, 152, 204, 275, 338, 278, 304, 457, 612, 612, 702, 307, 522, 724, 404, 674, 488, 487, 554, 625};
        int width[]    = {196, 142, 190, 264,  84,  84, 142, 189,  53,  53, 199, 139, 141,  51,  77, 140, 196, 189, 189, 255, 255, 121,  56,  56, 188};
        int height[]   = {196, 142, 190, 264,  84,  84, 142, 189,  53,  53, 199, 139, 141,  51,  77, 140, 196, 189, 114, 255, 162, 121,  56,  56, 188};
        for (int j = 0; j < 25; ++j)
        {
            Rectangle2D *rect = new Rectangle2D(Point2D(corner_x[j] * 1.0f, corner_y[j] * 1.0f),
                                                width[j] * 1.0f, height[j] * 1.0f, *colors[5],
                                                true);
            addObject2D(rect);
            background[i].push_back(rect);
        }
    }
    for (int i = 0; i < 4; ++i)
    {
        Rectangle2D *back_rect = new Rectangle2D(Point2D(-5, -5), 855, 855, *colors[7], true);
        addObject2D(back_rect);
        background[i].push_back(back_rect);
    }
    update_background();

    /* desenez obiectele vizibile initial */
    draw_visible_objects();
}

/* functia care permite animatia */
void DrawingWindow::onIdle()
{
    if (paused)
        return;

    /* sterg liniutele din explozie */
    if (dead && idle_calls - death_time >= EXP_TIME && explosion_angles.size())
    {
        vector<Object2D *> lines_to_delete(DrawingWindow::objects2D.begin() + first_explosion_pos,
                                           objects2D.begin() + 1 + last_explosion_pos);
        DrawingWindow::objects2D.erase(DrawingWindow::objects2D.begin() + first_explosion_pos,
                                       objects2D.begin() + 1 + last_explosion_pos);

        last_explosion_pos -= EXP_LINES;
        first_circle_pos   -= EXP_LINES;
        last_circle_pos    -= EXP_LINES;
        first_traj_pos     -= EXP_LINES;
        last_traj_pos      -= EXP_LINES;
        first_elem_pos     -= EXP_LINES;
        last_elem_pos      -= EXP_LINES;

        for (unsigned int i = 0; i < lines_to_delete.size(); ++i)
            delete lines_to_delete[i];
        explosion_angles.clear();
    }

    /* translatez liniutele din explozie */
    if (dead)
        for (int i = first_explosion_pos; i <= last_explosion_pos; ++i)
        {
            int line_num = i - first_explosion_pos;
            Transform2D::loadIdentityMatrix();
            Transform2D::translateMatrix(8 * (idle_calls - death_time) *
                                             cos(explosion_angles[line_num]),
                                         8 * (idle_calls - death_time) *
                                             sin(explosion_angles[line_num]));
            Transform2D::applyTransform(objects2D[i]);
        }

    Cell first, last;
    vector<Point2D> vertices;

    if (!dead)
    {
        /* calculez coordonatele varfurilor jucatorului */
        float sin_u = sin(player_angle);
        float cos_u = cos(player_angle);
        for (int p = -1; p <= 1; p += 2)
            for (int q = -1; q <= 1; q += 2)
                vertices.push_back(Point2D(21 * p * cos_u - 21 * q * sin_u + player.x,
                                           21 * p * sin_u + 21 * q * cos_u + player.y));

        /* identific zona din jurul jucatorului ce contine elementele cu care acesta ar putea
         * interactiona */
        first.col = int_trunc((player.x - 30) / 42);
        last.col = int_trunc((player.x + 30) / 42);
        first.row = objects.size() - int_trunc((player.y + 30) / 42) - 1;
        if (first.row < 0)
            first.row = 0;
        else if (first.row >= static_cast<int>(objects.size()))
            first.row = objects.size() - 1;
        last.row = objects.size() - int_trunc((player.y - 30) / 42) - 1;
        if (last.row < 0)
            last.row = 0;
        else if (last.row >= static_cast<int>(objects.size()))
            last.row = objects.size() - 1;
    }

    if (!dead && !game_won)
    {
        for (int i = first.row; i <= last.row && !dead && !game_won; ++i)
            for (int j = first.col; j <= last.col && !dead && !game_won; ++j)
            {
                int col = rem(j, objects[0].size());
                Point2D off(42.0f * j, 42.0f * (lines.size() - 1 - i));
                switch (lines[i][col])
                {
                    case 'A':
                    case 'a':
                        if (intersect_tri(vertices, off,
                                          Point2D(off.x + 21,
                                                  off.y + (lines[i][col] == 'A' ? 42 : 15)),
                                          Point2D(off.x + 42, off.y)))
                            die();
                    break;
                    case 'V':
                    case 'v':
                        if (intersect_tri(vertices,
                                          Point2D(off.x, off.y + 42),
                                          Point2D(off.x + 21,
                                                  off.y + (lines[i][col] == 'V' ? 0 : 27)),
                                          Point2D(off.x + 42, off.y + 42)))
                            die();
                    break;
                    case '>':
                    case ';':
                        if (intersect_tri(vertices,
                                          Point2D(off.x, off.y + 42),
                                          Point2D(off.x + (lines[i][col] == '>' ? 42 : 15),
                                          off.y + 21), off))
                            die();
                    break;
                    case '<':
                    case ':':
                        if (intersect_tri(vertices,
                                          Point2D(off.x + (lines[i][col] == '<' ? 0 : 27),
                                                  off.y + 21),
                                          Point2D(off.x + 42, off.y + 42),
                                          Point2D(off.x + 42, off.y)))
                            die();
                    break;
                    case '^':
                        if (intersect_rect(vertices, off, Point2D(off.x + 42, off.y + 11)))
                            die();
                    break;
                    case 'P':
                        if (!collected_flag(Cell(i, col)))
                        {
                            if (intersect_tri(vertices,
                                              Point2D(off.x + 3, off.y + 32),
                                              Point2D(off.x + 21, off.y + 42),
                                              Point2D(off.x + 21, off.y + 32)) ||
                                intersect_rect(vertices,
                                               Point2D(off.x + 3, off.y),
                                               Point2D(off.x + 21, off.y + 32)))
                            {
                                collected.push_back(Cell(i, col));
                                Text *new_score = new Text(
                                    "Puncte: " + to_string(collected.size()) + " / " +
                                                 to_string(FLAGS_NEEDED),
                                    Point2D((game_width - 101.0f) / 2 - 1, -1037),
                                    Color(1, 1, 1),
                                    BITMAP_HELVETICA_18);
                                DrawingWindow::texts[0] = new_score;
                                vector<Object2D *>::iterator it = find(objects2D.begin(),
                                                                       objects2D.end(),
                                                                       objects[i][col][0]);
                                objects2D.erase(it, it + objects[i][col].size());
                                last_elem_pos -= objects[i][col].size();
                                add_effect_circle(Point2D(off.x + 21, off.y + 21));
                                last_checkpoint = Cell(i, j);
                                if (collected.size() == FLAGS_NEEDED)
                                {
                                    game_won = true;
                                    Text *new_lifes = new Text(" ", Point2D(0, 0), Color(1, 1, 1),
                                                               BITMAP_HELVETICA_18);
                                    DrawingWindow::texts[1] = new_lifes;
                                    if (state == SEATED)
                                        if (player_line != objects.size() || elevated)
                                        {
                                            state = FALLING;
                                            coef_a = -0.40024f * 42 * speed * speed;
                                            coef_b = 1.850962f * 42 * speed;
                                            jump_start_t = idle_calls +
                                                static_cast<unsigned int>(coef_b / (coef_a * 2));
                                            jump_start_y = player.y - 2.14f * 42;
                                        }
                                        else
                                            last_non_won_col = last_seen_col;
                                }
                                else
                                {
                                    lifes = 3;
                                    Text *new_lifes = new Text("Incercari: 3",
                                                               Point2D(game_width - 92.0f, -1037),
                                                               Color(1, 1, 1),
                                                               BITMAP_HELVETICA_18);
                                    DrawingWindow::texts[1] = new_lifes;
                                }
                            }
                        }
                    break;
                    case '.':
                        if (Cell(i, j) != jump_elem)
                        {
                            bool hit = false;
                            if (i + 1 >= static_cast<int>(lines.size()) ||
                                lines[i + 1][col] == '#' || lines[i + 1][col] == '-' ||
                                lines[i + 1][col] == 'n' || lines[i + 1][col] == 'V' ||
                                lines[i + 1][col] == 'v')
                            {
                                if (intersect_tri(vertices, off, Point2D(off.x + 21, off.y + 10),
                                                  Point2D(off.x + 42, off.y)))
                                {
                                    coef_a = -0.423584f * 42 * speed * speed;
                                    coef_b =   2.71304f * 42 * speed;
                                    hit = true;
                                }
                            }
                            else
                            {
                                if (intersect_rect(vertices, Point2D(off.x + 14, off.y - 7),
                                                   Point2D(off.x + 28, off.y + 7)))
                                {
                                    coef_a = -0.408012f * 42 * speed * speed;
                                    coef_b =   2.02247f * 42 * speed;
                                    hit = true;
                                }
                            }
                            if (hit)
                            {
                                state = JUMPING;
                                auto_jump = true;
                                start_auto_jump = idle_calls;
                                jump_start_t = idle_calls;
                                jump_start_y = player.y;
                                jump_elem = Cell(i, j);
                                add_effect_circle(Point2D(off.x + 21, off.y));
                            }
                        }
                    break;
                }
            }
    }

    /* actualizez starea */
    if (!dead && (state == JUMPING || state == FALLING))
    {
        /* caut elementele solide de care s-ar putea lovi */
        int intersection_type = NO_INT;
        for (int i = first.row; i <= last.row && !game_won; ++i)
        {
            for (int j = first.col; j <= last.col && !game_won; ++j)
            {
                int col = rem(j, objects[0].size());
                Point2D left_bottom, right_top;
                left_bottom.x = j * 42.0f;
                right_top.x = (j + 1) * 42.0f;
                if (lines[i][col] == '#' || lines[i][col] == 'n')
                {
                    left_bottom.y = 42.0f * (lines.size() - 1 - i);
                    right_top.y = 42.0f * (lines.size() - i);
                }
                else if (lines[i][col] == '_')
                {
                    left_bottom.y = 42.0f * (lines.size() - 1 - i);
                    right_top.y = left_bottom.y + 17;
                }
                else if (lines[i][col] == '-')
                {
                    right_top.y = 42.0f * (lines.size() - i);
                    left_bottom.y = right_top.y - 17;
                }
                if (lines[i][col] == '#' || lines[i][col] == 'n' || lines[i][col] == '-' ||
                    lines[i][col] == '_')
                {
                    int temp = intersect_rect(vertices, left_bottom, right_top);
                    if (temp == INT_RIGHT)
                    {
                        intersection_type = temp;
                        break;
                    }
                    if (temp != NO_INT && !(state == JUMPING && temp == INT_DOWN) &&
                        !(state == FALLING && temp == INT_UP))
                    {
                        intersection_type = temp;
                        if (temp == INT_DOWN)
                            if (lines[i][col] == '_')
                            {
                                player_line = i + 1;
                                elevated = true;
                            }
                            else
                            {
                                player_line = i;
                                elevated = false;
                            }
                    }
                }
            }
            if (intersection_type == INT_RIGHT)
                break;
        }
        /* verific intersectia cu pamantul si "tavanul" */
        if (intersection_type != INT_RIGHT)
        {
            for (int i = 0; i < 4; ++i)
            {
                if (vertices[i].y < 0 && state != JUMPING)
                {
                    intersection_type = INT_DOWN;
                    player_line = lines.size();
                    elevated = false;
                }
                else if (vertices[i].y > lines.size() * 42 && state != FALLING)
                    intersection_type = INT_UP;
            }
        }
        if (intersection_type == INT_RIGHT)
            die();
        else if (intersection_type == INT_UP)
        {
            /* fac o translatie a parabolei in timp, incat sa para ca jucatorul s-a ciocnit de
             * tavan */
            state = FALLING;
            jump_start_t += static_cast<unsigned int>(sqrt(coef_b * coef_b +
                4 * coef_a * (player.y - jump_start_y)) / coef_a);
        }
        else if (intersection_type == INT_DOWN)
        {
            /* marchez ca jucatorul a picat pe o suprafata orizontala, ii corectez rotatia si ii
             * actualizez pozitia */
            state = SEATED;
            auto_jump = false;
            player_angle = real_modulo(player_angle, 2 * PI);
            if (player_angle < PI / 4)
                player_angle = 0;
            else if (player_angle < 3 * PI / 4)
                player_angle = PI / 2;
            else if (player_angle < 5 * PI / 4)
                player_angle = PI;
            else if (player_angle < 7 * PI / 4)
                player_angle = 3 * PI / 2;
            else
                player_angle = 0;
            player.y = (lines.size() - player_line) * 42.0f + 21;
            if (elevated)
                player.y += 17;
            if (game_won)
                last_non_won_col = last_seen_col;
            if (last_space && !game_won && idle_calls - last_space <= SPACE_DELAY)
            {
                state = JUMPING;
                jump_start_t = idle_calls;
                jump_start_y = player.y;
                last_space = 0;
                coef_a = -0.40024f * 42 * speed * speed;
                coef_b = 1.850962f * 42 * speed;
            }
        }
    }
    else if (!dead && !game_won)
    {
        /* singura coloana pe care s-ar putea afla ceva de care sa ma lovesc */
        int col = rem(int_trunc((player.x + 21) / 42), lines[0].size());
        if (!elevated)
        {
            if (lines[player_line - 1][col] == '#' || lines[player_line - 1][col] == 'n' ||
                lines[player_line - 1][col] == '_' || lines[player_line - 1][col] == '-')
                die();
        }
        else
        {
            if (lines[player_line - 2][col] == '#' || lines[player_line - 2][col] == 'n' ||
                lines[player_line - 2][col] == '_')
                die();
            if (lines[player_line - 1][col] == '#' || lines[player_line - 1][col] == 'n' ||
                lines[player_line - 1][col] == '-')
                die();
        }
        if (!dead)
        {
            /* Bula mergea pe un cal si dintr-o data cade. De ce? S-a terminat calul. */
            /* aici verific daca s-a terminat calul cum ar veni */
            bool end_of_support = false;
            int left_col = rem(int_trunc((player.x + 21) / 42) - 1, lines[0].size());
            if (!elevated)
            {
                if (player_line < static_cast<int>(lines.size()) &&
                    lines[player_line][left_col] != '#' && lines[player_line][left_col] != 'n' &&
                    lines[player_line][left_col] != '-' && lines[player_line][col] != '#' &&
                    lines[player_line][col] != 'n' && lines[player_line][col] != '-')
                    end_of_support = true;
            }
            else
            {
                if (player_line - 1 < static_cast<int>(lines.size()) &&
                    lines[player_line - 1][left_col] != '_' && lines[player_line - 1][col] != '_')
                    end_of_support = true;
            }
            if (end_of_support)
            {
                /* simulez o saritura care ar fi avut loc in trecut, incat sa urmeze din momentul
                 * asta doar partea de cadere */
                state = FALLING;
                coef_a = -0.40024f * 42 * speed * speed;
                coef_b = 1.850962f * 42 * speed;
                jump_start_t = idle_calls + static_cast<unsigned int>(coef_b / (coef_a * 2));
                jump_start_y = player.y - 2.14f * 42;
            }
        }
    }

    /* actualizez pozitia si unghiul jucatorului */
    Point2D init(player);
    if (!dead)
    {
        player.x += speed * 42;
        if (state == JUMPING || state == FALLING)
        {
            float t = static_cast<float>(idle_calls - jump_start_t);
            if (state == JUMPING && t > -coef_b / (coef_a * 2))
                state = FALLING;
            player.y = coef_a * t * t + coef_b * t + jump_start_y;
            player_angle += PI * coef_a / coef_b;
        }
    }

    /* aplic transformarile calculate mai sus */
    if (!dead)
    {
        Transform2D::loadIdentityMatrix();
        Transform2D::rotateMatrix(player_angle);
        Transform2D::translateMatrix(player.x, player.y);
        for (unsigned int elem = 0; elem < player_graphics.size(); ++elem)
            Transform2D::applyTransform(player_graphics[elem]);
    }

    /* daca sunt in timpul unei sarituri automate, desenez traiectoria */
    if (auto_jump && start_auto_jump != idle_calls)
    {
        objects2D.insert(objects2D.begin() + 1 + last_traj_pos,
                         new Line2D(init, player, Color(1, 1, 1)));
        ++last_traj_pos;
        ++first_elem_pos;
        ++last_elem_pos;
    }

    /* sterg din traiectorie, daca e prea veche */
    if (first_traj_pos <= last_traj_pos && idle_calls - start_auto_jump + 1 > MAX_TRAJ)
    {
        Line2D *line = (Line2D *)objects2D[first_traj_pos];
        objects2D.erase(objects2D.begin() + first_traj_pos);
        delete line;
        --last_traj_pos;
        --first_elem_pos;
        --last_elem_pos;
    }

    /* desenez cercurile unui efect vizual */
    list<Object2D *>::iterator it1 = circles.begin();
    list<Point2D>::iterator it2 = circle_positions.begin();
    list<unsigned int>::iterator it3 = circles_time.begin();
    for (; it1 != circles.end(); ++it1, ++it2, ++it3)
    {
        Transform2D::loadIdentityMatrix();
        Transform2D::translateMatrix(-it2->x, -it2->y);
        Transform2D::scaleMatrix(2.0f * (idle_calls - *it3), 2.0f * (idle_calls - *it3));
        Transform2D::translateMatrix(it2->x, it2->y);
        Transform2D::applyTransform(*it1);
    }

    /* sterg din cercurile din cadrul efectelor, daca sunt prea vechi */
    if (!circles.empty() && idle_calls - circles_time.front() + 1 > MAX_CIRCLE)
    {
        Circle2D *circle = (Circle2D *)objects2D[first_circle_pos];
        circles.erase(circles.begin());
        circle_positions.erase(circle_positions.begin());
        circles_time.erase(circles_time.begin());
        objects2D.erase(objects2D.begin() + first_circle_pos);
        delete circle;
        --last_circle_pos;
        --first_traj_pos;
        --last_traj_pos;
        --first_elem_pos;
        --last_elem_pos;
    }

    /* update camera */
    if (follow_player)
        camera.x = player.x - 6.5f * 42;
    else if (camera.x <= player.x - 6.5f * 42)
        follow_player = true;
    float diff_y = player.y - camera.y;
    if (diff_y <= 21) /* jucatorul e pe cale sa iasa din cadru (ducandu-se in jos) */
        camera.y = player.y - 22;
    else if (diff_y <= 145)
        camera.y -= (1 - (diff_y - 21) / 124) * speed * 100;
    else if (diff_y >= 429) /* jucatorul e pe cale sa iasa din cadru (ducandu-se in sus) */
        camera.y = player.y - 428;
    else if (diff_y >= 305)
        camera.y += (diff_y - 305) / 124 * speed * 100;
    game->fereastra(camera.x, camera.y, camera.x + 800, camera.y + 450);

    /* update pamant */
    while (first_tile != static_cast<int>((camera.x - ground_offset) / 200))
    {
        Transform2D::loadIdentityMatrix();
        Transform2D::translateMatrix(ground_offset + (first_tile + 5) * 200, 0);
        for (int i = 0; i < 6; ++i)
            Transform2D::applyTransform(ground[(first_tile % 5) * 6 + i]);
        ++first_tile;
    }

    update_background();

    /* update elemente vizibile */
    int new_first_seen_col;
    new_first_seen_col = int_trunc(camera.x / 42) + (real_modulo(camera.x, 42) >= 21 ? 1 : 0);
    if (new_first_seen_col > first_seen_col)
    {
        /* identific ultimul obiect care a disparut, ca sa sterg totul pana la el inclusiv */
        Object2D *last_object_to_delete = NULL;
        bool found = false;
        for (int j = new_first_seen_col - 1; j >= first_seen_col; --j)
        {
            int col = rem(j, objects[0].size());
            for (int i = objects.size() - 1; i >= 0; --i)
                if (objects[i][col].size() > 0 && !collected_flag(Cell(i, col)))
                {
                    last_object_to_delete = objects[i][col][objects[i][col].size() - 1];
                    found = true;
                    break;
                }
            if (found)
                break;
        }
        if (found)
        {
            for (int pos = first_elem_pos; pos < static_cast<int>(objects2D.size()); ++pos)
                if (objects2D[pos] == last_object_to_delete)
                {
                    last_elem_pos -= pos - first_elem_pos + 1;
                    objects2D.erase(objects2D.begin() + first_elem_pos,
                                    objects2D.begin() + 1 + pos);
                    break;
                }
        }
        first_seen_col = new_first_seen_col;
        while (!auto_jump_list.empty() && auto_jump_list.front().col < first_seen_col)
            auto_jump_list.erase(auto_jump_list.begin());
    }
    int new_last_seen_col = int_trunc((camera.x + 800) / 42) +
                            (real_modulo(camera.x + 800, 42) >= 21 ? 0 : -1);
    if (new_last_seen_col > last_seen_col)
    {
        for (int j = last_seen_col + 1; j <= new_last_seen_col; ++j)
        {
            int col = rem(j, objects[0].size());
            for (int i = 0; i < static_cast<int>(objects.size()); ++i)
                if (game_won && state == SEATED)
                {
                    if (won_column < 30 && i + 4 >= static_cast<int>(objects.size()))
                    {
                        int won_row = i - objects.size() + 4;
                        DrawingWindow::objects2D.insert(
                            DrawingWindow::objects2D.begin() + 1 + last_elem_pos,
                            won_matr_obj[won_row][won_column].begin(),
                            won_matr_obj[won_row][won_column].end());
                        last_elem_pos += won_matr_obj[won_row][won_column].size();
                    }
                }
                else if (!collected_flag(Cell(i, col)))
                {
                    DrawingWindow::objects2D.insert(
                        DrawingWindow::objects2D.begin() + 1 + last_elem_pos,
                        objects[i][col].begin(),
                        objects[i][col].end());
                    if ((lines[i][col] == '.') &&
                        !(i + 1 >= static_cast<int>(lines.size()) || lines[i + 1][col] == '#' ||
                            lines[i + 1][col] == '-' || lines[i + 1][col] == 'n' ||
                            lines[i + 1][col] == 'V' || lines[i + 1][col] == 'v'))
                        auto_jump_list.push_back(Cell(i, j));
                    last_elem_pos += objects[i][col].size();
                }
            if (game_won && state == SEATED)
                won_column++;
        }
        last_seen_col = new_last_seen_col;
    }

    /* update dimensiuni obiecte care dispar */
    for (int j = first_seen_col; j <= last_seen_col; ++j)
    {
        int col = rem(j, objects[0].size());
        if (42 * j > camera.x + 1.5 * 42)
            break;
        float scale = (42 * j + 21 - camera.x) / (2 * 42);
        if (scale < 0)
            scale = 0;
        else if (scale > 1)
            scale = 1;
        for (int i = 0; i < static_cast<int>(objects.size()); ++i)
            if (game_won && state == SEATED && j > last_non_won_col)
            {
                int new_col = j - last_non_won_col - 1;
                if (new_col < 30 && i + 4 >= static_cast<int>(objects.size()))
                {
                    int won_row = i - objects.size() + 4;
                    Transform2D::loadIdentityMatrix();
                    Transform2D::translateMatrix(-21, -21);
                    Transform2D::scaleMatrix(scale, scale);
                    Transform2D::translateMatrix(42.0f * j + 21,
                                                 42.0f * (objects.size() - 1 - i) + 21);
                    for (unsigned int elem = 0; elem < won_matr_obj[won_row][new_col].size();
                         ++elem)
                        Transform2D::applyTransform(won_matr_obj[won_row][new_col][elem]);
                }
                else if (new_col >= 30)
                    waiting_for_key = true;
            }
            else if (objects[i][col].size() > 0 && !collected_flag(Cell(i, col)))
            {
                Transform2D::loadIdentityMatrix();
                Transform2D::translateMatrix(-21, -21);
                Transform2D::scaleMatrix(scale, scale);
                Transform2D::translateMatrix(42.0f * j + 21, 42.0f * (objects.size() - 1 - i) + 21);
                for (unsigned int elem = 0; elem < objects[i][col].size(); ++elem)
                    Transform2D::applyTransform(objects[i][col][elem]);
            }
    }
    /* update dimensiuni obiecte care apar */
    for (int j = last_seen_col; j > first_seen_col + 1; --j)
    {
        int col = rem(j, objects[0].size());
        if (42 * j + 21 < camera.x + 800 - 2 * 42)
        {
            /* aici e prima coloana de obiecte care sunt suficient de in stanga, incat nu mai
             * trebuie scalate */
            for (int i = 0; i < static_cast<int>(objects.size()); ++i)
                if (game_won && state == SEATED && j > last_non_won_col)
                {
                    int new_col = j - last_non_won_col - 1;
                    if (new_col < 30 && i + 4 >= static_cast<int>(objects.size()))
                    {
                        int won_row = i - objects.size() + 4;
                        Transform2D::loadIdentityMatrix();
                        Transform2D::translateMatrix(-21, -21);
                        Transform2D::scaleMatrix(1, 1);
                        Transform2D::translateMatrix(42.0f * j + 21,
                                                     42.0f * (objects.size() - 1 - i) + 21);
                        for (unsigned int elem = 0; elem < won_matr_obj[won_row][new_col].size();
                             ++elem)
                            Transform2D::applyTransform(won_matr_obj[won_row][new_col][elem]);
                    }
                }
                else if (objects[i][col].size() > 0 && !collected_flag(Cell(i, col)))
                {
                    Transform2D::loadIdentityMatrix();
                    Transform2D::translateMatrix(-21, -21);
                    Transform2D::scaleMatrix(1, 1);
                    Transform2D::translateMatrix(42.0f * j + 21,
                                                 42.0f * (objects.size() - 1 - i) + 21);
                    for (unsigned int elem = 0; elem < objects[i][col].size(); ++elem)
                        Transform2D::applyTransform(objects[i][col][elem]);
                }
            break;
        }
        float scale = (camera.x + 800 - (42 * j + 21)) / (2 * 42);
        if (scale < 0)
            scale = 0;
        else if (scale > 1)
            scale = 1;
        for (int i = 0; i < static_cast<int>(objects.size()); ++i)
            if (game_won && state == SEATED && j > last_non_won_col)
            {
                int new_col = j - last_non_won_col - 1;
                if (new_col < 30 && i + 4 >= static_cast<int>(objects.size()))
                {
                    int won_row = i - objects.size() + 4;
                    Transform2D::loadIdentityMatrix();
                    Transform2D::translateMatrix(-21, -21);
                    Transform2D::scaleMatrix(scale, scale);
                    Transform2D::translateMatrix(42.0f * j + 21,
                                                 42.0f * (objects.size() - 1 - i) + 21);
                    for (unsigned int elem = 0; elem < won_matr_obj[won_row][new_col].size();
                         ++elem)
                        Transform2D::applyTransform(won_matr_obj[won_row][new_col][elem]);
                }
            }
            else if (objects[i][col].size() > 0 && !collected_flag(Cell(i, col)))
            {
                Transform2D::loadIdentityMatrix();
                Transform2D::translateMatrix(-21, -21);
                Transform2D::scaleMatrix(scale, scale);
                Transform2D::translateMatrix(42.0f * j + 21,
                                             42.0f * (objects.size() - 1 - i) + 21);
                for (unsigned int elem = 0; elem < objects[i][col].size(); ++elem)
                    Transform2D::applyTransform(objects[i][col][elem]);
            }
    }

    /* update dimensiuni elemente de saritura automata */
    list<Cell>::iterator it;
    for (it = auto_jump_list.begin(); it != auto_jump_list.end(); ++it)
    {
        int col = rem(it->col, objects[0].size());
        Transform2D::loadIdentityMatrix();
        Transform2D::translateMatrix(-21, 0);
        float marginal_scale1 = (camera.x + 800 - (42 * it->col + 21)) / (2 * 42);
        if (marginal_scale1 < 0)
            marginal_scale1 = 0;
        else if (marginal_scale1 > 1)
            marginal_scale1 = 1;
        float marginal_scale2 = (42 * it->col + 21 - camera.x) / (2 * 42);;
        if (marginal_scale2 < 0)
            marginal_scale2 = 0;
        else if (marginal_scale2 > 1)
            marginal_scale2 = 1;
        float final_scale = marginal_scale1 * marginal_scale2 *
                            (0.4f + (sin(0.1f * idle_calls) + 1) * 0.3f);
        Transform2D::scaleMatrix(final_scale, final_scale);
        Transform2D::translateMatrix(42.0f * it->col + 21, 42.0f * (objects.size() - 1 - it->row));
        for (unsigned int elem = 0; elem < objects[it->row][col].size(); ++elem)
            Transform2D::applyTransform(objects[it->row][col][elem]);
    }

    if (!waiting_for_key && dead && idle_calls - death_time >= EXP_TIME + ROUND_DELAY)
        start();

    ++idle_calls;
}

/* functia care se apeleaza la redimensionarea ferestrei */
void DrawingWindow::onReshape(int width, int height)
{
    bool show_scripts;
    int min_width;
    if (game_over)
        min_width = 2 * 91 + 88;
    else
        min_width = 2 * 91 + 101;
    if (width >= min_width && height >= 21)
    {
        int game_height;
        bool horizontal_space;
        if ((height - 21) * 16 / 9 < width)
        {
            game_width = (height - 21) * 16 / 9;
            game_height = (height - 21);
            horizontal_space = true;
        }
        else
        {
            game_width = width;
            game_height = width * 9 / 16;
            horizontal_space = false;
        }
        show_scripts = game_width >= 283;
        if (show_scripts)
        {
            if (!game_over)
            {
                Text *new_score = new Text(
                    "Puncte: " + to_string(collected.size()) + " / " + to_string(FLAGS_NEEDED),
                    Point2D((game_width - 101.0f) / 2 - 1, -1037), Color(1, 1, 1),
                    BITMAP_HELVETICA_18);
                texts[0] = new_score;
            }
            else
            {
                Text *lost = new Text("Ai pierdut.", Point2D((game_width - 88.0f) / 2 - 1, -1037),
                                      Color(1, 0.5f, 0.5f), BITMAP_HELVETICA_18);
                texts[0] = lost;
            }
            if (game_won)
            {
                Text *new_lifes = new Text(" ", Point2D(0, 0), Color(1, 1, 1), BITMAP_HELVETICA_18);
                texts[1] = new_lifes;
            }
            else if (collected.size())
            {
                Text *new_lifes = new Text("Incercari: " + to_string(lifes),
                                           Point2D(game_width - 92.0f, -1037), Color(1, 1, 1),
                                           BITMAP_HELVETICA_18);
                texts[1] = new_lifes;
            }
            script->fereastra(0, -1040, game_width * 1.0f, -1020);
            int left_limit = (width - game_width) / 2;
            int right_limit = (width + game_width) / 2;
            int upper_limit = (height - game_height - 21) / 2;
            script->poarta(left_limit, upper_limit, right_limit, upper_limit + 20);
            script_backgr->poarta(left_limit, upper_limit, right_limit, upper_limit + 20);
            game->poarta(left_limit, upper_limit + 21, right_limit, upper_limit + 21 + game_height);
            if (horizontal_space)
            {
                backgr1->poarta(0, 0, left_limit, height);
                backgr2->poarta(right_limit, 0, width, height);
            }
            else
            {
                backgr1->poarta(0, 0, width, upper_limit);
                backgr2->poarta(0, upper_limit + game_height, width, height);
            }
        }
    }
    else
        show_scripts = false;
    if (!show_scripts)
        if (height < width * 9 / 16)
        {
            game_width = height * 16 / 9;
            int left_limit = (width - game_width) / 2;
            int right_limit = (width + game_width) / 2;
            script->poarta(left_limit, -10, right_limit, -10);
            script_backgr->poarta(left_limit, -10, right_limit, -10);
            game->poarta(left_limit, 0, right_limit, height);
            backgr1->poarta(0, 0, left_limit, height);
            backgr2->poarta(right_limit, 0, width, height);
        }
        else
        {
            int game_height = width * 9 / 16;
            int upper_limit = (height - game_height) / 2;
            script->poarta(0, -10, width, -10);
            script_backgr->poarta(0, -10, width, -10);
            game->poarta(0, upper_limit, width, upper_limit + game_height);
            backgr1->poarta(0, 0, width, upper_limit);
            backgr2->poarta(0, upper_limit + game_height, width, height);
        }
}

/* functia care defineste ce se intampla cand se apasa pe tastatura */
void DrawingWindow::onKey(unsigned char key)
{
    if (!paused && game_won && !waiting_for_key && key != 27)
        return;
    if (!paused && waiting_for_key && key != 27)
    {
        start();
        return;
    }
    switch(key)
    {
        case 27 :
            if (!paused)
            {
                paused = true;
                Text *pause = new Text("Pauza", Point2D(2, -1037), Color(1, 1, 1),
                                       BITMAP_HELVETICA_18);
                texts[2] = pause;
            }
            else
            {
                paused = false;
                texts[2] = new Text(" ", Point2D(0, 0), Color(1, 1, 1), BITMAP_HELVETICA_18);
            }
        break;
        case ' ':
            if (!paused)
            {
                if (state == SEATED)
                {
                    state = JUMPING;
                    auto_jump = false;
                    jump_start_t = idle_calls;
                    jump_start_y = player.y;
                    coef_a = -0.40024f * 42 * speed * speed;
                    coef_b = 1.850962f * 42 * speed;
                }
                else
                    last_space = idle_calls;
            }
    }
}

int main(int argc, char** argv)
{
    /* initializarea vitezei de joc */
    if (argc > 1)
        speed = static_cast<float>(atof(argv[1]));
    else
        speed = 0.15f;
    /* creare fereastra */
    DrawingWindow dw(argc, argv, 800, 471, 200, 100, "Tema 1 EGC");
    /* se apeleaza functia init() - in care s-au adaugat obiecte */
    dw.init();
    /* se intra in bucla principala de desenare - care face posibila desenarea, animatia si
     * procesarea evenimentelor */
    dw.run();
    return 0;
}
