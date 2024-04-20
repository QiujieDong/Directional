//
// Created by Amir Vaxman on 20.04.24.
//

#ifndef DIRECTIONAL_GENERATED_MESH_SIMPLIFICATION_H
#define DIRECTIONAL_GENERATED_MESH_SIMPLIFICATION_H

#include <set>
#include <math.h>
#include <vector>
#include <queue>
#include <algorithm>
#include <utility>
#include <Eigen/Sparse>
#include <Eigen/Dense>
#include <gmp.h>
#include <gmpxx.h>
#include <directional/GMP_definitions.h>

namespace directional{

    class NFunctionMesher {
    public:

        struct EdgeData{
            int ID;
            bool isFunction;
            int OrigHalfedge;
            bool isBoundary;
            int funcNum;  //in case of function segment

            EdgeData():ID(-1), isFunction(false), OrigHalfedge(-1), isBoundary(false), funcNum(-1){}
            ~EdgeData(){};
        };

        class Vertex{
        public:
            int ID;
            Eigen::RowVector3d Coordinates;
            EVector3 ECoordinates;
            int AdjHalfedge;

            bool isFunction;
            bool Valid;

            Vertex():ID(-1), AdjHalfedge(-1), isFunction(false), Valid(true){}
            ~Vertex(){}
        };

        class Halfedge{
        public:
            int ID;
            int Origin;
            int Next;
            int Prev;
            int Twin;
            int AdjFace;
            Eigen::VectorXd NFunction;
            std::vector<ENumber> exactNFunction;
            bool isFunction;
            bool Valid;

            //Parametric function values
            int OrigHalfedge;
            int OrigNFunctionIndex;  //the original parameteric function assoicated with this edge
            //int prescribedAngleDiff;
            double prescribedAngle;  //the actual prescribed angle


            Halfedge():ID(-1), Origin(-1), Next(-1), Prev(-1), Twin(-1), AdjFace(-1), isFunction(false), Valid(true), OrigHalfedge(-1), OrigNFunctionIndex(-1),  prescribedAngle(-1.0){}
            ~Halfedge(){}
        };


        class Face{
        public:
            int ID;
            int AdjHalfedge;
            Eigen::RowVector3d Normal;
            Eigen::RowVector3d Centroid;
            Eigen::RowVector3d Basis1, Basis2;
            bool Valid;

            Face():ID(-1), AdjHalfedge(-1), Valid(true){}
            ~Face(){}
        };

        std::vector<Vertex> Vertices;
        std::vector<Halfedge> Halfedges;
        std::vector<Face> Faces;

        std::vector<int> TransVertices;
        std::vector<int> InStrip;
        std::vector<std::set<int> > VertexChains;



        struct MergeData {
            const bool operator()(const int &v1, const int &v2) const { return v1; }
        };

        bool JoinFace(constint heindex) {
            if (Halfedges[heindex].Twin < 0)
                return true;  //there is no joining of boundary faces

            int Face1 = Halfedges[heindex].AdjFace;
            int Face2 = Halfedges[Halfedges[heindex].Twin].AdjFace;

            int hebegin = Faces[Face1].AdjHalfedge;
            int heiterate = hebegin;
            do {
                heiterate = Halfedges[heiterate].Next;
            } while (heiterate != hebegin);

            hebegin = Faces[Face1].AdjHalfedge;
            heiterate = hebegin;
            do {
                heiterate = Halfedges[heiterate].Next;
            } while (heiterate != hebegin);

            //check if spike edge
            if ((Halfedges[heindex].Prev == Halfedges[heindex].Twin) ||
                (Halfedges[heindex].Next == Halfedges[heindex].Twin)) {


                int CloseEdge = heindex;
                if (Halfedges[heindex].Prev == Halfedges[heindex].Twin)
                    CloseEdge = Halfedges[heindex].Twin;

                Halfedges[CloseEdge].Valid = Halfedges[Halfedges[CloseEdge].Twin].Valid = false;

                Vertices[Halfedges[CloseEdge].Origin].AdjHalfedge = Halfedges[Halfedges[CloseEdge].Twin].Next;
                Faces[Face1].AdjHalfedge = Halfedges[CloseEdge].Prev;

                Halfedges[Halfedges[CloseEdge].Prev].Next = Halfedges[Halfedges[CloseEdge].Twin].Next;
                Halfedges[Halfedges[Halfedges[CloseEdge].Twin].Next].Prev = Halfedges[CloseEdge].Prev;

                Vertices[Halfedges[Halfedges[CloseEdge].Twin].Origin].Valid = false;
                //Faces[Face1].NumVertices-=2;  //although one vertex should appear twice


                int hebegin = Faces[Face1].AdjHalfedge;
                int heiterate = hebegin;
                do {

                    heiterate = Halfedges[heiterate].Next;
                } while (heiterate != hebegin);

                hebegin = Faces[Face1].AdjHalfedge;
                heiterate = hebegin;
                do {
                    heiterate = Halfedges[heiterate].Next;
                } while (heiterate != hebegin);


                return true;
            }

            if (Face1 == Face2)
                return false;  //we don't remove non-spike edges with the same faces to not disconnect a chain

            hebegin = Faces[Face2].AdjHalfedge;
            heiterate = hebegin;
            do {
                heiterate = Halfedges[heiterate].Next;
            } while (heiterate != hebegin);

            Faces[Face1].AdjHalfedge = Halfedges[heindex].Next;
            Faces[Face2].Valid = false;

            //Faces[Face2].AdjHalfedge=Halfedges[Halfedges[heindex].Twin].Next;

            Halfedges[heindex].Valid = Halfedges[Halfedges[heindex].Twin].Valid = false;

            Halfedges[Halfedges[heindex].Next].Prev = Halfedges[Halfedges[heindex].Twin].Prev;
            Halfedges[Halfedges[Halfedges[heindex].Twin].Prev].Next = Halfedges[heindex].Next;

            Halfedges[Halfedges[Halfedges[heindex].Twin].Next].Prev = Halfedges[heindex].Prev;
            Halfedges[Halfedges[heindex].Prev].Next = Halfedges[Halfedges[heindex].Twin].Next;

            Vertices[Halfedges[heindex].Origin].AdjHalfedge = Halfedges[Halfedges[heindex].Twin].Next;
            Vertices[Halfedges[Halfedges[heindex].Next].Origin].AdjHalfedge = Halfedges[heindex].Next;

            //all other floating halfedges should renounce this one
            for (int i = 0; i < Halfedges.size(); i++)
                if (Halfedges[i].AdjFace == Face2)
                    Halfedges[i].AdjFace = Face1;

            //Faces[Face1].NumVertices+=Faces[Face2].NumVertices-2;

            //DebugLog<<"Official number of vertices: "<<Faces[Face1].NumVertices;

            hebegin = Faces[Face1].AdjHalfedge;
            heiterate = hebegin;
            int currVertex = 0;
            do {
                //Faces[Face1].Vertices[currVertex++]=Halfedges[heiterate].Origin;
                heiterate = Halfedges[heiterate].Next;
            } while (heiterate != hebegin);

            return true;


        }

        void UnifyEdges(int heindex) {
            //if (Halfedges[heindex].Twin<0)
            //  return;
            //adjusting source
            Vertices[Halfedges[heindex].Origin].Valid = false;
            Halfedges[heindex].Origin = Halfedges[Halfedges[heindex].Prev].Origin;
            if (Halfedges[heindex].prescribedAngle < 0.0)
                Halfedges[heindex].prescribedAngle = Halfedges[Halfedges[heindex].Prev].prescribedAngle;
            Vertices[Halfedges[heindex].Origin].AdjHalfedge = heindex;

            Faces[Halfedges[heindex].AdjFace].AdjHalfedge = Halfedges[heindex].Next;
            //Faces[Halfedges[heindex].AdjFace].NumVertices--;



            //adjusting halfedges
            Halfedges[Halfedges[heindex].Prev].Valid = false;
            Halfedges[heindex].Prev = Halfedges[Halfedges[heindex].Prev].Prev;
            Halfedges[Halfedges[heindex].Prev].Next = heindex;

            //adjusting twin, if exists
            if (Halfedges[heindex].Twin >= 0) {
                if (Halfedges[Halfedges[heindex].Twin].prescribedAngle < 0.0)
                    Halfedges[Halfedges[heindex].Twin].prescribedAngle = Halfedges[Halfedges[Halfedges[heindex].Twin].Next].prescribedAngle;
                Halfedges[Halfedges[Halfedges[heindex].Twin].Next].Valid = false;
                Halfedges[Halfedges[heindex].Twin].Next = Halfedges[Halfedges[Halfedges[heindex].Twin].Next].Next;
                Halfedges[Halfedges[Halfedges[heindex].Twin].Next].Prev = Halfedges[heindex].Twin;
                Faces[Halfedges[Halfedges[heindex].Twin].AdjFace].AdjHalfedge = Halfedges[Halfedges[heindex].Twin].Next;
                //Faces[Halfedges[Halfedges[heindex].Twin].AdjFace].NumVertices--;
            }
        }

        bool CheckMesh(const bool verbose, const bool checkHalfedgeRepetition, const bool CheckTwinGaps,
                       const bool checkPureBoundary) {
            for (int i = 0; i < Vertices.size(); i++) {
                if (!Vertices[i].Valid)
                    continue;

                if (Vertices[i].AdjHalfedge == -1) {
                    if (verbose) std::cout << "Valid Vertex " << i << " points to non-valid value -1 " << std::endl;
                    return false;
                }

                if (!Halfedges[Vertices[i].AdjHalfedge].Valid) {
                    if (verbose)
                        std::cout << "Valid Vertex " << i << " points to non-valid AdjHalfedge "
                                  << Vertices[i].AdjHalfedge << std::endl;
                    return false;
                }


                if (Halfedges[Vertices[i].AdjHalfedge].Origin != i) {
                    if (verbose)
                        std::cout << "Adjacent Halfedge " << Vertices[i].AdjHalfedge << " of vertex " << i
                                  << "does not point back" << std::endl;
                    return false;
                }

            }

            for (int i = 0; i < Halfedges.size(); i++) {
                if (!Halfedges[i].Valid)
                    continue;


                if (Halfedges[i].Next == -1) {
                    if (verbose)
                        std::cout << "Valid Halfedge " << i << "points to Next non-valid value -1" << std::endl;
                    return false;
                }

                if (Halfedges[i].Prev == -1) {
                    if (verbose)
                        std::cout << "Valid Halfedge " << i << "points to Prev non-valid value -1" << std::endl;
                    return false;
                }


                if (Halfedges[i].Origin == -1) {
                    if (verbose)
                        std::cout << "Valid Halfedge " << i << "points to Origin non-valid value -1" << std::endl;
                    return false;
                }

                if (Halfedges[i].AdjFace == -1) {
                    if (verbose)
                        std::cout << "Valid Halfedge " << i << "points to AdjFace non-valid value -1" << std::endl;
                    return false;
                }

                if (Halfedges[Halfedges[i].Next].Prev != i) {
                    if (verbose)
                        std::cout << "Halfedge " << i << "Next is " << Halfedges[i].Next
                                  << " which doesn't point back as Prev" << std::endl;
                    return false;
                }


                if (Halfedges[Halfedges[i].Prev].Next != i) {
                    if (verbose)
                        std::cout << "Halfedge " << i << "Prev is " << Halfedges[i].Prev
                                  << " which doesn't point back as Next" << std::endl;
                    return false;
                }

                if (!Vertices[Halfedges[i].Origin].Valid) {
                    if (verbose)
                        std::cout << "The Origin of halfedges " << i << ", vertex " << Halfedges[i].Origin
                                  << " is not valid" << std::endl;
                    return false;
                }

                if (!Faces[Halfedges[i].AdjFace].Valid) {
                    if (verbose)
                        std::cout << "The face of halfedges " << i << ", face " << Halfedges[i].AdjFace
                                  << " is not valid" << std::endl;
                    return false;
                }

                if (Halfedges[Halfedges[i].Next].Origin == Halfedges[i].Origin) {  //a degenerate edge{
                    if (verbose)
                        std::cout << "Halfedge " << i << " with twin" << Halfedges[i].Twin
                                  << " is degenerate with vertex " << Halfedges[i].Origin << std::endl;
                    return false;
                }

                if (Halfedges[i].Twin >= 0) {
                    if (Halfedges[Halfedges[i].Twin].Twin != i) {
                        if (verbose)
                            std::cout << "Halfedge " << i << "twin is " << Halfedges[i].Twin
                                      << " which doesn't point back" << std::endl;
                        return false;
                    }

                    if (!Halfedges[Halfedges[i].Twin].Valid) {
                        if (verbose)
                            std::cout << "halfedge " << i << " is twin with invalid halfedge" << Halfedges[i].Twin
                                      << std::endl;
                        return false;
                    }
                }

                if (!Halfedges[Halfedges[i].Next].Valid) {
                    if (verbose)
                        std::cout << "halfedge " << i << " has next invalid halfedge" << Halfedges[i].Next << std::endl;
                    return false;
                }

                if (!Halfedges[Halfedges[i].Prev].Valid) {
                    if (verbose)
                        std::cout << "halfedge " << i << " has prev invalid halfedge" << Halfedges[i].Prev << std::endl;
                    return false;
                }

                if (Halfedges[i].isFunction) {  //checking that it is not left alone
                    if (Halfedges[i].Prev == Halfedges[i].Twin) {
                        if (verbose)
                            std::cout << "Hex halfedge " << i << " has Halfedge " << Halfedges[i].Prev
                                      << " and both prev and twin" << std::endl;
                        return false;
                    }


                    if (Halfedges[i].Next == Halfedges[i].Twin) {
                        if (verbose)
                            std::cout << "Hex halfedge " << i << " has Halfedge " << Halfedges[i].Next
                                      << " and both next and twin" << std::endl;
                        return false;
                    }
                }
            }

            std::vector <std::set<int>> halfedgesinFace(Faces.size());
            std::vector <std::set<int>> verticesinFace(Faces.size());
            for (int i = 0; i < Faces.size(); i++) {
                if (!Faces[i].Valid)
                    continue;

                //if (Faces[i].NumVertices<3)  //we never allow this
                //  return false;
                int hebegin = Faces[i].AdjHalfedge;
                int heiterate = hebegin;
                int NumEdges = 0;
                int actualNumVertices = 0;

                do {
                    if (verticesinFace[i].find(Halfedges[heiterate].Origin) != verticesinFace[i].end())
                        if (verbose)
                            std::cout << "Warning: Vertex " << Halfedges[heiterate].Origin
                                      << " appears more than once in face " << i << std::endl;

                    verticesinFace[i].insert(Halfedges[heiterate].Origin);
                    halfedgesinFace[i].insert(heiterate);
                    actualNumVertices++;
                    if (!Halfedges[heiterate].Valid)
                        return false;

                    if (Halfedges[heiterate].AdjFace != i) {
                        if (verbose)
                            std::cout << "Face " << i << " has halfedge " << heiterate << " that does not point back"
                                      << std::endl;
                        return false;
                    }

                    heiterate = Halfedges[heiterate].Next;
                    NumEdges++;
                    if (NumEdges > Halfedges.size()) {
                        if (verbose) std::cout << "Infinity loop!" << std::endl;
                        return false;
                    }


                } while (heiterate != hebegin);

                /*if (actualNumVertices!=Faces[i].NumVertices){
                  DebugLog<<"Faces "<<i<<" lists "<<Faces[i].NumVertices<<" vertices but has a chain of "<<actualNumVertices<<endl;
                  return false;
                }

                for (int j=0;j<Faces[i].NumVertices;j++){
                  if (Faces[i].Vertices[j]<0){
                    DebugLog<<"Faces "<<i<<".vertices "<<j<<"is undefined"<<endl;
                    return false;
                  }
                  if (!Vertices[Faces[i].Vertices[j]].Valid){
                    DebugLog<<"Faces "<<i<<".vertices "<<j<<"is not valid"<<endl;
                    return false;
                  }
                }*/
            }

            //checking if all halfedges that relate to a face are part of its recognized chain (so no floaters)
            for (int i = 0; i < Halfedges.size(); i++) {
                if (!Halfedges[i].Valid)
                    continue;
                int currFace = Halfedges[i].AdjFace;
                if (halfedgesinFace[currFace].find(i) == halfedgesinFace[currFace].end()) {
                    if (verbose) std::cout << "Halfedge " << i << " is floating in face " << currFace << std::endl;
                    return false;
                }
            }

            //check if mesh is a manifold: every halfedge appears only once
            if (checkHalfedgeRepetition) {
                std::set <TwinFinder> HESet;
                for (int i = 0; i < Halfedges.size(); i++) {
                    if (!Halfedges[i].Valid)
                        continue;
                    std::set<TwinFinder>::iterator HESetIterator = HESet.find(
                            TwinFinder(i, Halfedges[i].Origin, Halfedges[Halfedges[i].Next].Origin));
                    if (HESetIterator != HESet.end()) {
                        if (verbose)
                            std::cout << "Warning: the halfedge (" << Halfedges[i].Origin << ","
                                      << Halfedges[Halfedges[i].Next].Origin << ") appears at least twice in the mesh"
                                      << std::endl;
                        if (verbose)
                            std::cout << "for instance halfedges " << i << " and " << HESetIterator->index << std::endl;
                        return false;
                        //return false;
                    } else {
                        HESet.insert(TwinFinder(i, Halfedges[i].Origin, Halfedges[Halfedges[i].Next].Origin));
                        //if (verbose) std::cout<<"inserting halfedge "<<i<<" which is "<<Halfedges[i].Origin<<", "<<Halfedges[Halfedges[i].Next].Origin<<endl;
                    }
                }
            }

            if (CheckTwinGaps) {
                std::set <TwinFinder> HESet;
                //checking if there is a gap: two halfedges that share the same opposite vertices but do not have twins
                for (int i = 0; i < Halfedges.size(); i++) {
                    if (!Halfedges[i].Valid)
                        continue;

                    std::set<TwinFinder>::iterator HESetIterator = HESet.find(
                            TwinFinder(i, Halfedges[i].Origin, Halfedges[Halfedges[i].Next].Origin));
                    if (HESetIterator == HESet.end()) {
                        HESet.insert(TwinFinder(i, Halfedges[i].Origin, Halfedges[Halfedges[i].Next].Origin));
                        continue;
                    }

                    HESetIterator = HESet.find(TwinFinder(i, Halfedges[Halfedges[i].Next].Origin, Halfedges[i].Origin));
                    if (HESetIterator != HESet.end()) {

                        if (Halfedges[i].Twin == -1) {
                            if (verbose)
                                std::cout << "Halfedge " << i << "has no twin although halfedge "
                                          << HESetIterator->index << " can be a twin" << std::endl;
                            return false;
                        }
                        if (Halfedges[HESetIterator->index].Twin == -1) {
                            if (verbose)
                                std::cout << "Halfedge " << HESetIterator->index << "has no twin although halfedge "
                                          << i << " can be a twin" << std::endl;
                            return false;
                        }
                    }
                }
            }

            //checking if there are pure boundary faces (there shouldn't be)
            if (checkPureBoundary) {
                for (int i = 0; i < Halfedges.size(); i++) {
                    if (!Halfedges[i].Valid)
                        continue;
                    if ((Halfedges[i].Twin < 0) && (Halfedges[i].isFunction))
                        if (verbose)
                            std::cout << "WARNING: Halfedge " << i << " is a hex edge without twin!" << std::endl;

                    if (Halfedges[i].Twin > 0)
                        continue;

                    bool pureBoundary = true;
                    int hebegin = i;
                    int heiterate = hebegin;
                    do {
                        if (Halfedges[heiterate].Twin > 0) {
                            pureBoundary = false;
                            break;
                        }
                        heiterate = Halfedges[heiterate].Next;
                    } while (heiterate != hebegin);
                    if (pureBoundary) {
                        if (verbose)
                            std::cout << "Face " << Halfedges[i].AdjFace << "is a pure boundary face!" << std::endl;
                        return false;
                    }
                }

                //checking for latent valence 2 faces
                std::vector<int> Valences(Vertices.size());
                for (int i = 0; i < Vertices.size(); i++)
                    Valences[i] = 0;

                for (int i = 0; i < Halfedges.size(); i++) {
                    if (Halfedges[i].Valid) {
                        Valences[Halfedges[i].Origin]++;
                        //Valences[Halfedges[Halfedges[i].Next].Origin]++;
                        if (Halfedges[i].Twin < 0)  //should account for the target as well
                            Valences[Halfedges[Halfedges[i].Next].Origin]++;
                    }
                }

                int countThree;
                for (int i = 0; i < Faces.size(); i++) {
                    if (!Faces[i].Valid)
                        continue;
                    countThree = 0;
                    int hebegin = Faces[i].AdjHalfedge;
                    int heiterate = hebegin;
                    int numEdges = 0;
                    do {
                        if (Valences[Halfedges[heiterate].Origin] > 2)
                            countThree++;
                        heiterate = Halfedges[heiterate].Next;
                        numEdges++;
                        if (numEdges > Halfedges.size()) {
                            if (verbose) std::cout << "Infinity loop in face " << i << "!" << std::endl;
                            return false;
                        }
                    } while (heiterate != hebegin);
                    if (countThree < 3) {
                        if (verbose) std::cout << "Face " << i << " is a latent valence 2 face!" << std::endl;
                        if (verbose) std::cout << "Its vertices are " << std::endl;
                        do {
                            if (verbose)
                                std::cout << "Vertex " << Halfedges[heiterate].Origin << " halfedge " << heiterate
                                          << " valence " << Valences[Halfedges[heiterate].Origin] << std::endl;

                            if (Valences[Halfedges[heiterate].Origin] > 2)
                                countThree++;
                            heiterate = Halfedges[heiterate].Next;
                            numEdges++;
                            if (numEdges > Halfedges.size()) {
                                if (verbose) std::cout << "Infinity loop in face " << i << "!" << std::endl;
                                return false;
                            }
                        } while (heiterate != hebegin);

                        //return false;
                    }
                }
            }

            if (verbose) std::cout << "Mesh is clear according to given checks" << std::endl;
            return true;  //most likely the mesh is solid

        }

        void CleanMesh() {
            //removing nonvalid vertices
            std::vector<int> TransVertices(Vertices.size());
            std::vector <Vertex> NewVertices;
            for (int i = 0; i < Vertices.size(); i++) {
                if (!Vertices[i].Valid)
                    continue;

                Vertex NewVertex = Vertices[i];
                NewVertex.ID = NewVertices.size();
                NewVertices.push_back(NewVertex);
                TransVertices[i] = NewVertex.ID;
            }


            Vertices = NewVertices;
            for (int i = 0; i < Halfedges.size(); i++)
                Halfedges[i].Origin = TransVertices[Halfedges[i].Origin];



            /*for (int i=0;i<Faces.size();i++){
              if (!Faces[i].Valid)
                continue;
              for (int j=0;j<Faces[i].NumVertices;j++){
                DebugLog<<"i is "<<i<<", j is "<<j<<", Faces[i].Vertices[j] is "<<Faces[i].Vertices[j]<<endl;
                Faces[i].Vertices[j]=TransVertices[Faces[i].Vertices[j]];
              }
            }*/



            //removing nonvalid faces
            std::vector <Face> NewFaces;
            std::vector<int> TransFaces(Faces.size());
            for (int i = 0; i < Faces.size(); i++) {
                if (!Faces[i].Valid)
                    continue;

                Face NewFace = Faces[i];
                NewFace.ID = NewFaces.size();
                NewFaces.push_back(NewFace);
                TransFaces[i] = NewFace.ID;
            }
            Faces = NewFaces;
            for (int i = 0; i < Halfedges.size(); i++)
                Halfedges[i].AdjFace = TransFaces[Halfedges[i].AdjFace];



            //removing nonvalid halfedges
            std::vector <Halfedge> NewHalfedges;
            std::vector<int> TransHalfedges(Halfedges.size());
            for (int i = 0; i < Halfedges.size(); i++) {
                if (!Halfedges[i].Valid)
                    continue;

                Halfedge NewHalfedge = Halfedges[i];
                NewHalfedge.ID = NewHalfedges.size();
                NewHalfedges.push_back(NewHalfedge);
                TransHalfedges[i] = NewHalfedge.ID;
            }


            Halfedges = NewHalfedges;
            for (int i = 0; i < Faces.size(); i++)
                Faces[i].AdjHalfedge = TransHalfedges[Faces[i].AdjHalfedge];


            for (int i = 0; i < Vertices.size(); i++)
                Vertices[i].AdjHalfedge = TransHalfedges[Vertices[i].AdjHalfedge];


            for (int i = 0; i < Halfedges.size(); i++) {
                if (Halfedges[i].Twin != -1)
                    Halfedges[i].Twin = TransHalfedges[Halfedges[i].Twin];
                Halfedges[i].Next = TransHalfedges[Halfedges[i].Next];
                Halfedges[i].Prev = TransHalfedges[Halfedges[i].Prev];
            }

        }

        void ComputeTwins() {
            //twinning up edges
            std::set <TwinFinder> Twinning;
            for (int i = 0; i < Halfedges.size(); i++) {
                if (Halfedges[i].Twin >= 0)
                    continue;

                std::set<TwinFinder>::iterator Twinit = Twinning.find(
                        TwinFinder(0, Halfedges[Halfedges[i].Next].Origin, Halfedges[i].Origin));
                if (Twinit != Twinning.end()) {
                    Halfedges[Twinit->index].Twin = i;
                    Halfedges[i].Twin = Twinit->index;
                    Twinning.erase(*Twinit);
                } else {
                    Twinning.insert(TwinFinder(i, Halfedges[i].Origin, Halfedges[Halfedges[i].Next].Origin));
                }
            }
        }

        void WalkBoundary(int &CurrEdge) {
            do {
                CurrEdge = Halfedges[CurrEdge].Next;
                if (Halfedges[CurrEdge].Twin < 0)
                    break;  //next boundary over a 2-valence vertex
                CurrEdge = Halfedges[CurrEdge].Twin;
            } while (Halfedges[CurrEdge].Twin >= 0);
        }

        void RemoveVertex(int vindex, std::deque<int> &removeVertexQueue) {
            int hebegin = Vertices[vindex].AdjHalfedge;
            int heiterate = hebegin;
            do {
                if (heiterate == -1) {  //boundary vertex
                    return;
                }
                heiterate = Halfedges[Halfedges[heiterate].Prev].Twin;
            } while (heiterate != hebegin);

            Vertices[vindex].Valid = false;

            int remainingFace = Halfedges[hebegin].AdjFace;


            Faces[remainingFace].AdjHalfedge = Halfedges[hebegin].Next;
            heiterate = hebegin;
            int infinityCounter = 0;
            do {

                int NextEdge = Halfedges[heiterate].Next;
                int PrevEdge = Halfedges[Halfedges[heiterate].Twin].Prev;

                Halfedges[NextEdge].Prev = PrevEdge;
                Halfedges[PrevEdge].Next = NextEdge;
                if (Halfedges[NextEdge].AdjFace != remainingFace)
                    Faces[Halfedges[NextEdge].AdjFace].Valid = false;

                if (Halfedges[PrevEdge].AdjFace != remainingFace)
                    Faces[Halfedges[PrevEdge].AdjFace].Valid = false;


                Halfedges[PrevEdge].AdjFace = Halfedges[NextEdge].AdjFace = remainingFace;
                Halfedges[heiterate].Valid = false;
                Halfedges[Halfedges[heiterate].Twin].Valid = false;
                heiterate = Halfedges[Halfedges[heiterate].Prev].Twin;
                infinityCounter++;
                if (infinityCounter > Halfedges.size())
                    return;

            } while (heiterate != hebegin);

            //cleaning new face
            hebegin = Faces[remainingFace].AdjHalfedge;
            //Faces[remainingFace].NumVertices=0;
            heiterate = hebegin;
            infinityCounter = 0;
            do {
                //Faces[remainingFace].NumVertices++;
                Halfedges[heiterate].AdjFace = remainingFace;
                Vertices[Halfedges[heiterate].Origin].AdjHalfedge = heiterate;
                removeVertexQueue.push_front(Halfedges[heiterate].Origin);
                infinityCounter++;
                if (infinityCounter > Halfedges.size())
                    return;

                heiterate = Halfedges[heiterate].Next;
            } while (heiterate != hebegin);
        }


        void TestUnmatchedTwins();

        struct PointPair{
            int Index1, Index2;
            ENumber Distance;

            PointPair(int i1, int i2, ENumber d):Index1(i1), Index2(i2), Distance(d){}
            ~PointPair(){}

            const bool operator<(const PointPair& pp) const {
                if (Distance>pp.Distance) return false;
                if (Distance<pp.Distance) return true;

                if (Index1>pp.Index1) return false;
                if (Index1<pp.Index1) return true;

                if (Index2>pp.Index2) return false;
                if (Index2<pp.Index2) return true;

                return false;

            }
        };

        std::vector<std::pair<int,int>> FindVertexMatch(const bool verbose, std::vector<EVector3>& Set1, std::vector<EVector3>& Set2)
        {
            std::set<PointPair> PairSet;
            for (int i=0;i<Set1.size();i++)
                for (int j=0;j<Set2.size();j++)
                    PairSet.insert(PointPair(i,j,squaredDistance(Set1[i],Set2[j])));

            if (Set1.size()!=Set2.size())  //should not happen anymore
                std::cout<<"FindVertexMatch(): The two sets are of different sizes!! "<<std::endl;

            //adding greedily legal connections until graph is full
            std::vector<bool> Set1Connect(Set1.size());
            std::vector<bool> Set2Connect(Set2.size());

            std::vector<std::pair<int, int> > Result;

            for (int i=0;i<Set1.size();i++)
                Set1Connect[i]=false;

            for (int i=0;i<Set2.size();i++)
                Set2Connect[i]=false;

            /*if (Set1.size()!=Set2.size())
             int kaka=9;*/

            int NumConnected=0;

            //categorically match both ends

            Result.push_back(std::pair<int, int>(0,0));
            Result.push_back(std::pair<int, int>(Set1.size()-1,Set2.size()-1));
            for (std::set<PointPair>::iterator ppi=PairSet.begin();ppi!=PairSet.end();ppi++)
            {
                PointPair CurrPair=*ppi;
                //checking legality - if any of one's former are connected to ones latters or vice versa
                bool FoundConflict=false;
                for (int i=0;i<Result.size();i++){
                    if (((Result[i].first>CurrPair.Index1)&&(Result[i].second<CurrPair.Index2))||
                        ((Result[i].first<CurrPair.Index1)&&(Result[i].second>CurrPair.Index2))){
                        FoundConflict=true;
                        break;
                    }
                }

                if (FoundConflict)
                    continue;

                //if both are already matched, this matching is redundant
                if ((Set1Connect[CurrPair.Index1])&&(Set2Connect[CurrPair.Index2]))
                    continue;  //there is no reason for this matching

                //otherwise this edge is legal, so add it
                Result.push_back(std::pair<int, int>(CurrPair.Index1, CurrPair.Index2));
                if (!Set1Connect[CurrPair.Index1]) NumConnected++;
                if (!Set2Connect[CurrPair.Index2]) NumConnected++;
                Set1Connect[CurrPair.Index1]=Set2Connect[CurrPair.Index2]=true;
                /*if (NumConnected==Set1.size()+Set2.size())
                 break;  //all nodes are connected*/
            }

            for (int i=0;i<Set1.size();i++)
                if ((!Set1Connect[i])&&(verbose))
                    std::cout<<"Relative Vertex "<<i<<" in Set1 is unmatched!"<<std::endl;

            for (int i=0;i<Set2.size();i++)
                if ((!Set2Connect[i])&&(verbose))
                    std::cout<<"Relative Vertex "<<i<<" in Set2 is unmatched!"<<std::endl;

            /*if (NumConnected!=Set1.size()+Set2.size())
             int kaka=9;*/

            if (verbose){
                for (int i=0;i<Result.size();i++){
                    if (squaredDistance(Set1[Result[i].first],Set2[Result[i].second])>0){
                        std::cout<<"("<<Result[i].first<<","<<Result[i].second<<") with dist "<<squaredDistance(Set1[Result[i].first],Set2[Result[i].second])<<std::endl;
                        std::cout<<"Distance is abnormally not zero!"<<std::endl;
                    }
                }
            }


            return Result;

        }


        struct TwinFinder{
            int index;
            int v1,v2;

            TwinFinder(int i, int vv1, int vv2):index(i), v1(vv1), v2(vv2){}
            ~TwinFinder(){}

            const bool operator<(const TwinFinder& tf) const
            {
                if (v1<tf.v1) return false;
                if (v1>tf.v1) return true;

                if (v2<tf.v2) return false;
                if (v2>tf.v2) return true;

                return false;
            }


        };


        bool SimplifyMesh(const bool verbose, int N){
            //unifying vertices which are similar

            using namespace std;
            using namespace Eigen;

            if (!CheckMesh(verbose, false, false, false))
                return false;

            int MaxOrigHE=-3276700.0;
            for (int i=0;i<Halfedges.size();i++)
                MaxOrigHE=std::max(MaxOrigHE, Halfedges[i].OrigHalfedge);

            vector<bool> visitedOrig(MaxOrigHE+1);
            for (int i=0;i<MaxOrigHE+1;i++) visitedOrig[i]=false;
            for (int i=0;i<Halfedges.size();i++){
                if (Halfedges[i].OrigHalfedge<0)
                    continue;
                if (visitedOrig[Halfedges[i].OrigHalfedge])
                    continue;

                int hebegin = i;
                int heiterate = hebegin;
                do{
                    visitedOrig[Halfedges[heiterate].OrigHalfedge]=true;
                    WalkBoundary(heiterate);
                }while (heiterate!=hebegin);

            }

            vector< vector<int> > BoundEdgeCollect1(MaxOrigHE+1);
            vector< vector<int> > BoundEdgeCollect2(MaxOrigHE+1);
            vector<bool> Marked(Halfedges.size());
            for (int i=0;i<Halfedges.size();i++) Marked[i]=false;
            //finding out vertex correspondence along twin edges of the original mesh by walking on boundaries
            for (int i=0;i<Halfedges.size();i++){
                if ((Halfedges[i].OrigHalfedge<0)||(Marked[i]))
                    continue;

                //find the next beginning of a boundary
                int PrevOrig;
                int CurrEdge=i;
                do{
                    PrevOrig=Halfedges[CurrEdge].OrigHalfedge;
                    WalkBoundary(CurrEdge);
                }while(PrevOrig==Halfedges[CurrEdge].OrigHalfedge);

                //filling out strips of boundary with the respective attached original halfedges
                int BeginEdge=CurrEdge;
                vector<pair<int,int> > CurrEdgeCollect;
                do{
                    CurrEdgeCollect.push_back(pair<int, int> (Halfedges[CurrEdge].OrigHalfedge, CurrEdge));
                    Marked[CurrEdge]=true;
                    WalkBoundary(CurrEdge);
                }while (CurrEdge!=BeginEdge);

                PrevOrig=-1000;
                bool In1;
                for (int j=0;j<CurrEdgeCollect.size();j++){
                    if (CurrEdgeCollect[j].first!=PrevOrig)
                        In1=BoundEdgeCollect1[CurrEdgeCollect[j].first].empty();

                    if (In1)
                        BoundEdgeCollect1[CurrEdgeCollect[j].first].push_back(CurrEdgeCollect[j].second);
                    else
                        BoundEdgeCollect2[CurrEdgeCollect[j].first].push_back(CurrEdgeCollect[j].second);
                    PrevOrig=CurrEdgeCollect[j].first;
                }
            }

            //editing the edges into two vector lists per associated original edge
            vector< vector<int> > VertexSets1(MaxOrigHE+1), VertexSets2(MaxOrigHE+1);
            for (int i=0;i<MaxOrigHE+1;i++){
                for (int j=0;j<BoundEdgeCollect1[i].size();j++)
                    VertexSets1[i].push_back(Halfedges[BoundEdgeCollect1[i][j]].Origin);

                if (BoundEdgeCollect1[i].size()>0)
                    VertexSets1[i].push_back(Halfedges[Halfedges[BoundEdgeCollect1[i][BoundEdgeCollect1[i].size()-1]].Next].Origin);

                for (int j=0;j<BoundEdgeCollect2[i].size();j++)
                    VertexSets2[i].push_back(Halfedges[BoundEdgeCollect2[i][j]].Origin);

                if (BoundEdgeCollect2[i].size()>0)
                    VertexSets2[i].push_back(Halfedges[Halfedges[BoundEdgeCollect2[i][BoundEdgeCollect2[i].size()-1]].Next].Origin);

                std::reverse(VertexSets2[i].begin(),VertexSets2[i].end());
            }

            //finding out vertex matches
            vector<pair<int, int> > VertexMatches;
            for (int i=0;i<MaxOrigHE+1;i++){
                vector<EVector3> PointSet1(VertexSets1[i].size());
                vector<EVector3> PointSet2(VertexSets2[i].size());
                for (int j=0;j<PointSet1.size();j++)
                    PointSet1[j]=Vertices[VertexSets1[i][j]].ECoordinates;

                for (int j=0;j<PointSet2.size();j++)
                    PointSet2[j]=Vertices[VertexSets2[i][j]].ECoordinates;

                vector<pair<int, int> > CurrMatches;
                if ((!PointSet1.empty())&&(!PointSet2.empty()))
                    CurrMatches=FindVertexMatch(verbose, PointSet1, PointSet2);

                for (int j=0;j<CurrMatches.size();j++){
                    CurrMatches[j].first =VertexSets1[i][CurrMatches[j].first];
                    CurrMatches[j].second=VertexSets2[i][CurrMatches[j].second];
                }

                VertexMatches.insert(VertexMatches.end(), CurrMatches.begin(), CurrMatches.end() );
            }

            //finding connected components, and uniting every component into a random single vertex in it (it comes out the last mentioned)
            /*Graph MatchGraph;
            for (int i=0;i<Vertices.size();i++)
                add_vertex(MatchGraph);
            for (int i=0;i<VertexMatches.size();i++)
                add_edge(VertexMatches[i].first, VertexMatches[i].second, MatchGraph);*/

            double MaxDist=-327670000.0;
            for (int i=0;i<VertexMatches.size();i++)
                MaxDist=std::max(MaxDist, (Vertices[VertexMatches[i].first].Coordinates-Vertices[VertexMatches[i].second].Coordinates).squared_length());

            if (verbose)
                std::cout<<"Max matching distance: "<<MaxDist<<endl;

            //vector<int> TransVertices(Vertices.size());
            TransVertices.resize(Vertices.size());
            int NumNewVertices = connectedComponents(VertexMatches, TransVertices);

            if (!CheckMesh(verbose, false, false, false))
                return false;

            vector<bool> transClaimed(NumNewVertices);
            for (int i=0;i<NumNewVertices;i++)
                transClaimed[i]=false;
            //unifying all vertices into the TransVertices
            vector<Vertex> NewVertices(NumNewVertices);
            for (int i=0;i<Vertices.size();i++){  //redundant, but not terrible
                if (!Vertices[i].Valid)
                    continue;
                Vertex NewVertex=Vertices[i];
                NewVertex.ID=TransVertices[i];
                transClaimed[TransVertices[i]]=true;
                NewVertices[TransVertices[i]]=NewVertex;
            }

            for (int i=0;i<NumNewVertices;i++)
                if (!transClaimed[i])
                    NewVertices[i].Valid=false;  //this vertex is dead to begin with

            Vertices=NewVertices;

            for (int i=0;i<Halfedges.size();i++){
                if (!Halfedges[i].Valid)
                    continue;
                Halfedges[i].Origin=TransVertices[Halfedges[i].Origin];
                Vertices[Halfedges[i].Origin].AdjHalfedge=i;
            }


            if (!CheckMesh(verbose, true, false, false))
                return false;

            //twinning up edges
            set<TwinFinder> Twinning;
            for (int i=0;i<Halfedges.size();i++){
                if ((Halfedges[i].Twin>=0)||(!Halfedges[i].Valid))
                    continue;

                set<TwinFinder>::iterator Twinit=Twinning.find(TwinFinder(0,Halfedges[Halfedges[i].Next].Origin, Halfedges[i].Origin));
                if (Twinit!=Twinning.end()){
                    if ((Halfedges[Twinit->index].Twin!=-1)&&(verbose))
                        std::cout<<"warning: halfedge "<<Twinit->index<<" is already twinned to halfedge "<<Halfedges[Twinit->index].Twin<<std::endl;
                    if ((Halfedges[i].Twin!=-1)&&(verbose))
                        std::cout<<"warning: halfedge "<<i<<" is already twinned to halfedge "<<Halfedges[Twinit->index].Twin<<std::endl;
                    Halfedges[Twinit->index].Twin=i;
                    Halfedges[i].Twin=Twinit->index;

                    if (Halfedges[i].isFunction){
                        Halfedges[Twinit->index].isFunction = true;
                    } else if (Halfedges[Twinit->index].isFunction){
                        Halfedges[i].isFunction = true;
                    }
                    Twinning.erase(*Twinit);
                } else {
                    Twinning.insert(TwinFinder(i,Halfedges[i].Origin,Halfedges[Halfedges[i].Next].Origin));
                }
            }

            //check if there are any non-twinned edge which shouldn't be in a closed mesh
            /*if (verbose){
             for (int i=0;i<Halfedges.size();i++){
               if (Halfedges[i].Twin==-1)
                 std::cout<<"Halfedge "<<i<<" does not have a twin!"<<std::endl;
             }
            }*/


            if (!CheckMesh(verbose, true, true, true))
                return false;

            //removing triangle components

            //starting with pure triangle vertices
            std::vector<bool> isPureTriangle(Vertices.size());
            std::vector<bool> isBoundary(Vertices.size());
            for (int i=0;i<Vertices.size();i++){
                isPureTriangle[i]=true;
                isBoundary[i]=false;
            }
            for (int i=0;i<Halfedges.size();i++){
                if ((Halfedges[i].isFunction)&&(Halfedges[i].Valid)){
                    isPureTriangle[Halfedges[i].Origin]=isPureTriangle[Halfedges[Halfedges[i].Next].Origin]=false;  //adjacent to at least one hex edge
                }
                if (Halfedges[i].Twin==-1){
                    isBoundary[Halfedges[i].Origin]=true;
                    isPureTriangle[Halfedges[i].Origin]=false;  //this shouldn't be removed
                }
            }

            std::vector<bool> isEar(Vertices.size());
            for (int i=0;i<Vertices.size();i++){
                isEar[i] = (Halfedges[Vertices[i].AdjHalfedge].Twin==-1)&&(Halfedges[Halfedges[Vertices[i].AdjHalfedge].Prev].Twin==-1);
                if (isEar[i]) isPureTriangle[i]=false;
            }

            //realigning halfedges in hex vertices to only follow other hex edges
            for (int i=0;i<Vertices.size();i++){
                if ((isPureTriangle[i])||(!Vertices[i].Valid))
                    continue;

                vector<int> hexHEorder;
                int hebegin = Vertices[i].AdjHalfedge;
                if (isBoundary[i]){
                    //finding the first hex halfedge
                    while (Halfedges[Halfedges[hebegin].Prev].Twin!=-1)
                        hebegin =Halfedges[Halfedges[hebegin].Prev].Twin;
                }

                int heiterate=hebegin;
                do{
                    if ((Halfedges[heiterate].isFunction)||(Halfedges[heiterate].Twin==-1))
                        hexHEorder.push_back(heiterate);
                    if (Halfedges[heiterate].Twin==-1)
                        break;
                    heiterate = Halfedges[Halfedges[heiterate].Twin].Next;
                }while(heiterate!=hebegin);


                for (int j=0;j<hexHEorder.size();j++){
                    if ((isBoundary[i])&&(j==hexHEorder.size()-1))
                        continue;
                    Halfedges[hexHEorder[(j+1)%hexHEorder.size()]].Prev =Halfedges[hexHEorder[j]].Twin;
                    Halfedges[Halfedges[hexHEorder[j]].Twin].Next =hexHEorder[(j+1)%hexHEorder.size()];
                    Vertices[Halfedges[hexHEorder[j]].Origin].AdjHalfedge=hexHEorder[j];
                }

                if (isBoundary[i]){ //connect first to the prev
                    Halfedges[hexHEorder[0]].Prev = Halfedges[hebegin].Prev;
                    Halfedges[Halfedges[hebegin].Prev].Next =hexHEorder[0];
                    Vertices[Halfedges[hexHEorder[0]].Origin].AdjHalfedge=hexHEorder[0];
                }
            }

            //invalidating all triangle vertices and edges
            for (int i=0;i<Vertices.size();i++)
                if (isPureTriangle[i])
                    Vertices[i].Valid=false;

            for (int i=0;i<Halfedges.size();i++)
                if ((!Halfedges[i].isFunction)&&(Halfedges[i].Twin!=-1))
                    Halfedges[i].Valid=false;

            //realigning faces
            VectorXi visitedHE=VectorXi::Zero(Halfedges.size());
            VectorXi usedFace=VectorXi::Zero(Faces.size());
            for (int i=0;i<Halfedges.size();i++){
                if ((!Halfedges[i].Valid)||(visitedHE[i]!=0))
                    continue;

                //following the loop and reassigning face
                int currFace=Halfedges[i].AdjFace;
                Faces[currFace].AdjHalfedge=i;
                usedFace[currFace]=1;
                int hebegin=i;
                int heiterate=hebegin;
                int infinityCounter=0;
                do{
                    infinityCounter++;
                    if (infinityCounter>Halfedges.size()){
                        std::cout<<"Infinity loop in realigning faces on halfedge "<<i<<std::endl;
                        return false;
                    }
                    Halfedges[heiterate].AdjFace=currFace;
                    heiterate=Halfedges[heiterate].Next;
                }while (heiterate!=hebegin);
            }

            int countThree=0;
            for (int i=0;i<Faces.size();i++)
                if (!usedFace[i])
                    Faces[i].Valid=false;


            //killing perfect ear faces (not doing corners atm)
            //counting valences
            vector<int> Valences(Vertices.size());
            for (int i=0;i<Vertices.size();i++)
                Valences[i]=0;

            for (int i=0;i<Halfedges.size();i++){
                if (Halfedges[i].Valid){
                    Valences[Halfedges[i].Origin]++;
                    //Valences[Halfedges[Halfedges[i].Next].Origin]++;
                    if (Halfedges[i].Twin<0)  //should account for the target as well
                        Valences[Halfedges[Halfedges[i].Next].Origin]++;
                }
            }

            for (int i=0;i<Faces.size();i++){
                if (!Faces[i].Valid)
                    continue;
                countThree=0;
                int hebegin = Faces[i].AdjHalfedge;
                int heiterate=hebegin;
                do{
                    if (Valences[Halfedges[heiterate].Origin]>2)
                        countThree++;
                    heiterate=Halfedges[heiterate].Next;
                }while (heiterate!=hebegin);
                if (countThree<3){
                    do{
                        /*DebugLog<<"Invalidating Vertex "<<Halfedges[heiterate].Origin<<"and  halfedge "<<heiterate<<" of valence "<<Valences[Halfedges[heiterate].Origin]<<endl;*/

                        Halfedges[heiterate].Valid=false;
                        if (Halfedges[heiterate].Twin!=-1)
                            Halfedges[Halfedges[heiterate].Twin].Twin=-1;
                        if ((Halfedges[heiterate].Twin==-1)&&(Halfedges[Halfedges[heiterate].Prev].Twin==-1))  //origin is a boundary vertex
                            Vertices[Halfedges[heiterate].Origin].Valid=false;

                        heiterate=Halfedges[heiterate].Next;


                    }while (heiterate!=hebegin);
                    Faces[i].Valid=false;
                    //return false;
                }
            }

            //need to realign all vertices pointing
            for (int i=0;i<Halfedges.size();i++)
                if (Halfedges[i].Valid)
                    Vertices[Halfedges[i].Origin].AdjHalfedge=i;


            if (!CheckMesh(verbose, true, true, true))
                return false;

            for (int i=0;i<Valences.size();i++)
                if ((Vertices[i].Valid)&&(Valences[i]<2))
                    Vertices[i].Valid=false;

            for (int i=0;i<Vertices.size();i++){
                if ((Vertices[i].Valid)&&(Valences[i]<=2)&&(!isEar[i]))
                    UnifyEdges(Vertices[i].AdjHalfedge);
            }

            if (!CheckMesh(verbose, true, true, true))
                return false;

            //remove non-valid components
            CleanMesh();

            //checking if mesh is valid
            if (!CheckMesh(verbose, true, true, true))
                return false;

            return true;

        }

        void RemoveDegree2Faces();

        void Allocate(int NumofVertices, int NumofFaces, int NumofHEdges)
        {
            Vertices.resize(NumofVertices);
            Faces.resize(NumofFaces);
            Halfedges.resize(NumofHEdges);
        }


    };


}

#endif //DIRECTIONAL_GENERATED_MESH_SIMPLIFICATION_H
