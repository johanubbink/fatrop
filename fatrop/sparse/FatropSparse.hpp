/**
 * @file FatropSparse.hpp
 * @author your name (you@domain.com)
 * @brief this file contains functions for representing a block-sparse KKT matrix, only used for DEBUG and TESTING purposes. This code does NOT aim to be efficient, neither in computational nor memory efficiency.
 * @version 0.1
 * @date 2021-11-17
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#ifndef FATROP_SPARSE_INCLUDED
#define FATROP_SPARSE_INCLUDED
#include "ocp/OCPKKT.hpp"
#include "debug/LinearAlgebraEigen.hpp"
#include <vector>
#include <memory>
#include <cstdlib>
#include <string>
#include <iomanip> // std::setprecision
#include <eigen3/Eigen/Sparse>
using namespace std;
namespace fatrop
{
    /* TODO -> I'm not happy with how expressions are built up here (using shared pointers). We should make use of nodes that are reference counted, I also want scalarnode-expressions, which allow matrix operations like in casADi 
    class elem
    node*
    upon destruction node->ref_count -- if 0 -> free
    copy -> node_ref_count ++;


    class sum: node
    members:
    ref_count 
    elem1
    elem2

    elem operator+(elem1, elem2)
    elem res
    res.node = new sum(1,2)
    return res
    */
    struct Triplet
    {
        Triplet(const int ai, const int aj, const double value) : ai(ai), aj(aj), val(value){};
        int row() const { return ai; };
        int col() const { return aj; };
        double value() const { return val; };
        int ai;
        int aj;
        double val;
    };
    class Variable
    {
    public:
        Variable(int size) : size(size){};
        int size;
        int offset;
        void set_grad(const vector<double> &grad_)
        {
            grad.assign(size, 0.0);
            for (int i = 0; i < size; i++)
            {
                grad.at(i) = grad_.at(i);
            }
        };
        void add_rhs(vector<double> &rhs)
        {
            for (int i = 0; i < size; i++)
            {
                rhs.at(offset + i) = grad.at(i);
            }
        }
        vector<double> grad;
    };
    typedef shared_ptr<Variable> var_sp;
    class SparseKKTMatrixBase
    {
    };
    class MatrixVectorBase
    {
    public:
        MatrixVectorBase(const Eig &mat, var_sp var) : fsm(mat), var(var){};
        Eig fsm;
        var_sp var;
    };

    class SparseExpression
    {
    public:
        virtual bool is_matrix_vector() { return false; };
        virtual void add_to_mv_vec(vector<MatrixVectorBase> &mv_vec) = 0;
        virtual int get_size() = 0;
    };
    typedef shared_ptr<SparseExpression> fe_sp;
    class Equation
    {
    public:
        Equation(int size) : size(size){};
        int size;
        int offset;
        void add_expression(const fe_sp &express, vector<double> &rhs_)
        {
            rhs = rhs_;
            express->add_to_mv_vec(mv_veceq);
        }
        void add_triplets(vector<Triplet> &tripl, int offs_H)
        {
            for (unsigned long int i = 0; i < mv_veceq.size(); i++)
            {
                MatrixVectorBase mvi = mv_veceq.at(i);
                int offs_var = mvi.var->offset;
                Eig &fsm = mvi.fsm;
                int m = fsm.nrows();
                int n = fsm.ncols();
                for (int i = 0; i < m; i++)
                {
                    for (int j = 0; j < n; j++)
                    {
                        double val = fsm.get_el(i, j);
                        // if (!fsm.iszero(i, j))
                        ///// TODO THIS IS A DANGEROUS AND BAD WAY OF REPRESENTING SPARSITY WITHIN MATRICES
                        if (val!=0.0)
                        {

                            tripl.push_back(Triplet(offs_H + offset + i, offs_var + j, val));
                        }
                    }
                }
            }
        }
        void add_rhs(vector<double> &rhs_, int offs_H)
        {
            for (int i = 0; i < size; i++)
            {
                rhs_.at(offs_H + offset + i) = rhs.at(i);
            }
        }
        vector<MatrixVectorBase> mv_veceq;
        vector<double> rhs;
    };
    typedef std::shared_ptr<Equation> eq_sp;

    class FatropSum1 : public SparseExpression
    {
    public:
        FatropSum1(const fe_sp &fe1, const fe_sp &fe2) : child1(fe1), child2(fe2){};
        void add_to_mv_vec(vector<MatrixVectorBase> &mv_vec)
        {
            child1->add_to_mv_vec(mv_vec);
            child2->add_to_mv_vec(mv_vec);
        };
        int get_size()
        {
            return child1->get_size();
        }
        const fe_sp child1;
        const fe_sp child2;
    };
    fe_sp operator+(const fe_sp &fe1, const fe_sp &fe2);
    class MatrixVector : public SparseExpression, public MatrixVectorBase
    {
    public:
        MatrixVector(const Eig &mat, var_sp var) : MatrixVectorBase(mat, var){};
        bool is_matrix_vector() { return true; };
        void add_to_mv_vec(vector<MatrixVectorBase> &mv_vec)
        {
            MatrixVectorBase *mvb = static_cast<MatrixVectorBase *>(this);
            mv_vec.push_back(*mvb);
        }
        int get_size()
        {
            return fsm.nrows();
        }
    };
    fe_sp operator*(const Eig &mat, var_sp var);

    class HessBlock
    {
    public:
        HessBlock(const Eig &mat, var_sp var1, var_sp var2) : fsm(mat), var1(var1), var2(var2){};
        Eig fsm;
        var_sp var1;
        var_sp var2;
        void add_triplets(vector<Triplet> &tripl)
        {
            int offs_var1 = var1->offset;
            int offs_var2 = var2->offset;
            int m = fsm.nrows();
            int n = fsm.ncols();
            for (int i = 0; i < m; i++)
            {
                for (int j = 0; j < n; j++)
                {
                    double val = fsm.get_el(i, j);
                    // if (!fsm.iszero(i, j)) // only add nonzero's
                    ///// TODO THIS IS A DANGEROUS AND BAD WAY OF REPRESENTING SPARSITY WITHIN MATRICES
                    if (val!=0.0) // only add nonzero's
                    {
                        int ai = i + offs_var1;
                        int aj = j + offs_var2;
                        if (ai >= aj) // only lower triangular matrix
                        {
                            tripl.push_back(Triplet(ai, aj, val));
                        }
                    }
                    else
                    {
                    }
                }
            }
        }
    };

    class SparseKKTMatrix : public SparseKKTMatrixBase
    {
    public:
        var_sp get_variable(int size)
        {
            var_sp var = make_shared<Variable>(size);
            variable_vec.push_back(var);
            return variable_vec.back();
        };
        eq_sp get_equation(int size)
        {
            eq_sp eq = make_shared<Equation>(size);
            equation_vec.push_back(eq);
            return eq;
        };
        eq_sp set_equation(fe_sp expr, vector<double> rhs)
        {
            eq_sp eq = get_equation(expr->get_size());
            eq->add_expression(expr, rhs);
            return eq;
        }
        void set_hess_block(const Eig &mat, var_sp var1, var_sp var2)
        {
            HessBlock hb(mat, var1, var2);
            hess_block_vec.push_back(hb);
        }

        vector<var_sp> variable_vec;
        vector<eq_sp> equation_vec;
        vector<HessBlock> hess_block_vec;
        void set_offsets()
        {
            int offs_curr = 0;
            for (long unsigned int i = 0; i < variable_vec.size(); i++)
            {
                variable_vec.at(i)->offset = offs_curr;
                offs_curr += variable_vec.at(i)->size;
            }
            offs_curr = 0;
            for (long unsigned int i = 0; i < equation_vec.size(); i++)
            {
                equation_vec.at(i)->offset = offs_curr;
                offs_curr += equation_vec.at(i)->size;
            }
        }
        // TODO vector<triple> get_triplets, easier to use
        void get_triplets(vector<Triplet> &tripl_vec)
        {
            // tripl_vec.resize(0);
            this->set_offsets();
            for (long unsigned int i = 0; i < hess_block_vec.size(); i++)
            {
                hess_block_vec.at(i).add_triplets(tripl_vec);
            }
            int offs_vars = variable_vec.back()->offset + variable_vec.back()->size;
            // std::cout << "offs "<< offs_vars << std::endl;
            for (long unsigned int i = 0; i < equation_vec.size(); i++)
            {
                equation_vec.at(i)->add_triplets(tripl_vec, offs_vars);
            }
        }
        vector<double> get_rhs()
        {
            vector<double> rhs;
            this->set_offsets();
            rhs.assign(get_size(), 0.0);
            for (long unsigned int i = 0; i < variable_vec.size(); i++)
            {
                variable_vec.at(i)->add_rhs(rhs);
            }
            int offs_vars = variable_vec.back()->offset + variable_vec.back()->size;
            // cout << "offs "<< offs_vars << std::endl;
            for (long unsigned int i = 0; i < equation_vec.size(); i++)
            {
                equation_vec.at(i)->add_rhs(rhs, offs_vars);
            }
            return rhs;
        }
        int get_size()
        {
            set_offsets();
            return equation_vec.back()->offset + equation_vec.back()->size + variable_vec.back()->offset + variable_vec.back()->size;
        }

        void print(const char *type_id)
        {
            vector<Triplet> testvec;
            get_triplets(testvec);
            //printing (matrix)
            if (string(type_id) == "matrix")
            {
                cout << " " << get_size() << " " << get_size() << endl
                     << " ";
                cout << fixed;

                Eigen::MatrixXd mat(get_size(), get_size() + 1);

                for (long unsigned int i = 0; i < testvec.size(); i++)
                {
                    // std::cout << testvec.at(i).col() + 1 << " " << testvec.at(i).row()  << " " << std::setprecision(20) << testvec.at(i).value() << std::endl
                    mat(testvec.at(i).row(), testvec.at(i).col()) = testvec.at(i).value();
                    mat(testvec.at(i).col(), testvec.at(i).row()) = testvec.at(i).value();
                }
                vector<double> rhs = get_rhs();
                for (int i = 0; i < get_size(); i++)
                {
                    mat(i, get_size()) = rhs.at(i);
                }
                cout << mat << endl;
            }
            if (string(type_id) == "numpy")
            {
                cout << " " << get_size() << " " << get_size() << endl
                     << " ";
                cout << fixed;

                Eigen::MatrixXd mat(get_size(), get_size() + 1);

                for (long unsigned int i = 0; i < testvec.size(); i++)
                {
                    // std::cout << testvec.at(i).col() + 1 << " " << testvec.at(i).row()  << " " << std::setprecision(20) << testvec.at(i).value() << std::endl
                    mat(testvec.at(i).row(), testvec.at(i).col()) = testvec.at(i).value();
                    mat(testvec.at(i).col(), testvec.at(i).row()) = testvec.at(i).value();
                }
                vector<double> rhs = get_rhs();
                for (int i = 0; i < get_size(); i++)
                {
                    mat(i, get_size()) = rhs.at(i);
                }
                cout << "[";
                int size = get_size();
                for(int i = 0; i<size; i++){
                    cout << "[";
                for(int j = 0; j< size+1; j++){
                    cout << mat(i,j);
                    if(j!= size){cout << ", ";};
                }
                 cout << "]" ;
                 if(i!= size-1){cout << ",";};
                }
                cout << "]" << endl;
            }
            //printing (ma57)
            if (string(type_id) == "ma57")
            {
                cout << " " << get_size() << " " << testvec.size() << endl
                     << " ";
                cout << fixed;

                for (long unsigned int i = 0; i < testvec.size(); i++)
                {
                    cout << testvec.at(i).col() + 1 << " " << testvec.at(i).row() + 1 << " " << setprecision(20) << testvec.at(i).value() << endl
                         << " ";
                }
                vector<double> rhs = get_rhs();
                for (int i = 0; i < get_size(); i++)
                {
                    cout << rhs.at(i) << " ";
                }
                cout << endl;
            }
            //printing (MUMPS)
            if (string(type_id) == "mumps")
            {
                cout << get_size() << "     :N \n"
                     << testvec.size() << "     :NZ ";
                cout << fixed;

                for (long unsigned int i = 0; i < testvec.size(); i++)
                {
                    cout << endl
                         << testvec.at(i).col() + 1 << " " << testvec.at(i).row() + 1 << " " << setprecision(20) << testvec.at(i).value();
                }
                cout << "            :values";
                vector<double> rhs = get_rhs();
                for (int i = 0; i < get_size(); i++)
                {
                    cout << endl
                         << rhs.at(i);
                }
                cout << "            :RHS";
                cout << endl;
            }
        }
    };

} // namespace fatrop

#endif //FATROP_SPARSE_INCLUDED