// This file is part of Directional, a library for directional field processing.
// Copyright (C) 2018 Amir Vaxman <avaxman@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public License
// v. 2.0. If a copy of the MPL was not distributed with this file, You can
// obtain one at http://mozilla.org/MPL/2.0/.
#ifndef DIRECTIONAL_WRITE_MATCHING_H
#define DIRECTIONAL_WRITE_MATCHING_H
#include <cmath>
#include <Eigen/Core>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fstream>


namespace directional
{
    //TODO: this file is not updated to the tangent-bundle paradigm.
    // Writes the vector field matching into a file. For the file format specification see: https://avaxman.github.io/Directional/file_formats/
    // Inputs:
    //   fileName:  The to be loaded file.
    //   matching:  The matching per edge
    //   EF:        The edge to face matching
    //   EV:        The edge to vertices matching
    //   N:         The degree of the field
    // Return:
    //   Whether or not the file was written successfully
    bool IGL_INLINE write_matching(const std::string &fileName,
                                   const Eigen::VectorXi& matching,
                                   const Eigen::MatrixXi& EF,
                                   const Eigen::MatrixXi& EV,
                                   const Eigen::MatrixXi& FE,
                                   int N)
    {
        try
        {
            std::ofstream f(fileName);

            if (! f.is_open())
                return false;

            int numEdges = EF.rows();
            int numFaces = FE.rows();

            f << N << " " << numEdges << " " << numFaces << std::endl;
            for (int i = 0; i < numEdges; i++)
                f << EF(i,0) << " " << EF(i,1) << " " << EV(i, 0) << " " << EV(i, 1) << " "<< matching(i) << std::endl;

            for (int i=0;i<numFaces;i++)
                f << FE(i,0) <<" "<<FE(i,1)<<" "<<FE(i,2)<<std::endl;

            f.close();
            return f.fail();
        }
        catch (std::exception e)
        {
            return false;
        }
        return true;
    }
}

#endif
