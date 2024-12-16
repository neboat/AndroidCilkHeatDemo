//
// Created by TB Schardl on 12/13/24.
//

#ifndef CILKHEATDEMO2_SIM_H
#define CILKHEATDEMO2_SIM_H

#include "common.h"

// Data structure which holds the simulation state.
// To compare with the Lab 3 handout,
//  X and Y correspond to M and L from the lab handout
//  DT, DX, DY correspond to \DeltaT, \DeltaX, and \DeltaY
class SimState {
public:
    int X = 0;  // Dimensions of the grid for the simulation.
    int Y = 0;
    int TStep = 0;

    // Derive these arguments from X, Y, and TStep.
    double X1 = 0.0;  // Conceptual size of the grid.
    double Y1 = 0.0;
    double T1 = 0.0;
    double DT = 0.0, DX = 0.0, DY = 0.0;

    double CX = 0.0;  // CX = alpha * DT / DX^2
    double CY = 0.0;  // CY = alpha * DT / DY^2

    int Xsep = 0;    // Dimensions of the array to allocate.
    int Ysep = 0;    // This may be rounded up to nearst block size multiple.

    double* u = nullptr; // the 2*Xsep*Ysep array to store the values.
    char* raster = nullptr;  //here is the heat source pattern

    float heat_inc = 0.0;  //heat increased at each positive point in the pattern

    SimState(int x_sep, int y_sep, bool zero_init)
        : Xsep(BlockRound(x_sep)), Ysep(BlockRound(y_sep)) {
        if (zero_init) {
            u = (double *) calloc(GridSize(x_sep, y_sep) * 2, sizeof(double));
            raster = (char *) calloc(GridSize(x_sep, y_sep), sizeof(char));
        } else {
            u = (double*)malloc(GridSize(x_sep, y_sep) * 2 * sizeof(double));
            raster = (char*)malloc(GridSize(x_sep, y_sep) * sizeof(char));
        }
    }

    ~SimState() {
        free(u);
        free(raster);
    }

    void clear_raster_array() {
        memset(raster, 0, GridSize(Xsep, Ysep) * sizeof(char));
    }
    void clear() {
        clear_raster_array();
        memset(u, 0, GridSize(Xsep, Ysep) * sizeof(double));
    }

    // Takes values of X, Y, and TStep from params,
    // and sets appropriate values in Q.
    void set_sim_size(int X, int Y, int TStep) {
        this->X = X;
        this->Y = Y;
        this->TStep = TStep;

        // Derive the rest of the arguments
        X1 = X * Xmul;
        Y1 = Y * Ymul;
        DX = (X1 - X0) / (X - 1);
        DY = (Y1 - Y0) / (Y - 1);

        // Jim: I believe we need something like
        // 1 - 2*CX - 2*CY > 0 for stability?
        //
        // (I believe closer to 1 is more accurate?)
        //
        DT = (0.125 / alpha) * min((DX * DX), (DY * DY));
        T1 = T0 + DY;
        CX = alpha * DT / (DX * DX);
        CY = alpha * DT / (DY * DY);
    }

    // Applies the kernel for a single timestep.
    void kernel_inline(int t, int x, int y) const {
        if (x == 0 || x == X - 1 || y == 0 || y == Y - 1) {
            U(this, t + 1, x, y) = 0.0;
        } else
            U(this, t + 1, x, y) = alpha * (CX * (U(this, t, x + 1, y) - 2.0 * U(this, t, x, y) + U(this, t, x - 1, y))
                                         + CY * (U(this, t, x, y + 1) - 2.0 * U(this, t, x, y) + U(this, t, x, y - 1)))
                                + U(this, t, x, y);

        // add the heat
        U(this, t + 1, x, y) += heat_inc * Raster(this, y, x);
    }

    void null_kernel(int t, int x, int y) const {
        U(this, t + 1, x, y) += heat_inc * Raster(this, y, x);
    }

    void rect_null(int t0, int t1,
                   int x0, int x1,
                   int my_y0, int my_y1) const {
        for (int t = t0; t < t1; t++) {
            for (int x = x0; x < x1; x++) {
                int y;
                for (y = my_y0; y + 7 < my_y1; y += 8) {
                    null_kernel(t, x, y);
                    null_kernel(t, x, y + 1);
                    null_kernel(t, x, y + 2);
                    null_kernel(t, x, y + 3);
                    null_kernel(t, x, y + 4);
                    null_kernel(t, x, y + 5);
                    null_kernel(t, x, y + 6);
                    null_kernel(t, x, y + 7);
                }
                for (; y < my_y1; y++) {
                    null_kernel(t, x, y);
                }
            }
        }
    }
    // Jim: This is only here because Cilk++ crashes if LG_B > 1
    // In this case, we use the more complicated indexing macro, and
    // maybe Cilk++ can not inline that much?
#if (LG_B == 1)
#define kernel kernel_inline
#else
static void kernel_no_inline(int t, int x, int y) {
  kernel_inline(t, x, y);
}
#define kernel kernel_no_inline
#endif

    void kernel_single_timestep(int t, int my_x0, int my_x1, int my_y0, int my_y1) const {
        for (int x = my_x0; x < my_x1; ++x) {
            for (int y = my_y0; y < my_y1; ++y) {
                kernel(t, x, y);
            }
        }
    }

    void base_case_kernel(int t0, int t1, int x0, int dx0, int x1,
                          int dx1, int y0, int dy0, int y1, int dy1) const {
        for (int t = t0; t < t1; t++) {
            kernel_single_timestep(t, x0, x1, y0, y1);
            /* because the shape is trapezoid */
            x0 += dx0;
            x1 += dx1;
            y0 += dy0;
            y1 += dy1;
        }
    }
};

#endif //CILKHEATDEMO2_SIM_H
