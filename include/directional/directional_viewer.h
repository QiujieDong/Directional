// This file is part of Directional, a library for directional field processing.
// Copyright (C) 2020 Amir Vaxman <avaxman@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public License
// v. 2.0. If a copy of the MPL was not distributed with this file, You can
// obtain one at http://mozilla.org/MPL/2.0/.
#ifndef DIRECTIONAL_VIEWER_H
#define DIRECTIONAL_VIEWER_H

#include <Eigen/Core>
#include <igl/jet.h>
#include <igl/parula.h>
#include <igl/opengl/glfw/Viewer.h>
#include <directional/glyph_lines_mesh.h>
#include <directional/singularity_spheres.h>
#include <directional/seam_lines.h>
#include <directional/edge_diamond_mesh.h>
#include <directional/branched_isolines.h>
#include <directional/bar_chain.h>
#include <directional/halfedge_highlights.h>
#include <igl/edge_topology.h>


//This file contains the default libdirectional visualization paradigms
namespace directional
  {
  
#define NUMBER_OF_SUBMESHES 7
#define FIELD_MESH 1
#define SING_MESH 2
#define SEAMS_MESH 3
#define STREAMLINE_MESH 4
#define EDGE_DIAMOND_MESH 5
#define ISOLINES_MESH 6
  
  class DirectionalViewer: public igl::opengl::glfw::Viewer{
  private:
    std::vector<Eigen::MatrixXd> VList;  //vertices of mesh list
    std::vector<Eigen::MatrixXi> FList;  //faces of mesh list
    std::vector<Eigen::MatrixXi> EFList;  //edge information for the mesh
    std::vector<Eigen::MatrixXi> FEList;
    std::vector<Eigen::MatrixXi> EVList;
    
    std::vector<Eigen::MatrixXd> edgeVList;  //edge-diamond vertices list
    std::vector<Eigen::MatrixXi> edgeFList;  //edge-diamond faces list
    std::vector<Eigen::VectorXi> edgeFEList;  //edge-diamond faces->original mesh edges list
    
    std::vector<int> N;              //degrees of fields
    std::vector<Eigen::MatrixXd> fieldVList;
    std::vector<Eigen::MatrixXi> fieldFList;
    
  public:
    DirectionalViewer(){}
    ~DirectionalViewer(){}
    
    void IGL_INLINE set_mesh(const Eigen::MatrixXd& V,
                             const Eigen::MatrixXi& F,
                             const Eigen::MatrixXi& _EV=Eigen::MatrixXi(),
                             const Eigen::MatrixXi& _FE=Eigen::MatrixXi(),
                             const Eigen::MatrixXi& _EF=Eigen::MatrixXi(),
                             const int meshNum=0)
    {
      Eigen::MatrixXd meshColors;
      meshColors=default_mesh_color();
      Eigen::MatrixXi EV,FE,EF;
      if (_EV.rows()==0){
        igl::edge_topology(V,F,EV,FE,EF);
      } else{
        EV=_EV; FE=_FE; EF=_EF;
      }
      
      if (NUMBER_OF_SUBMESHES*(meshNum+1)>data_list.size()){  //allocating until there
        int currDLSize=data_list.size();
        for (int i=currDLSize;i<NUMBER_OF_SUBMESHES*(meshNum+1);i++)
          append_mesh();
      }
        
      selected_data_index=NUMBER_OF_SUBMESHES*meshNum;  //the last triangle mesh
      data_list[NUMBER_OF_SUBMESHES*meshNum].clear();
      data_list[NUMBER_OF_SUBMESHES*meshNum].set_mesh(V,F);
      if ((V.rows()==F.rows())||(meshColors.rows()!=V.rows()))  //assume face_based was meant
        data_list[NUMBER_OF_SUBMESHES*meshNum].set_face_based(true);
      data_list[NUMBER_OF_SUBMESHES*meshNum].set_colors(meshColors);
      data_list[NUMBER_OF_SUBMESHES*meshNum].show_lines=false;
      
      
      if (VList.size()<meshNum+1){
        VList.resize(meshNum+1);
        FList.resize(meshNum+1);
        edgeVList.resize(meshNum+1);
        edgeFList.resize(meshNum+1);
        edgeFEList.resize(meshNum+1);
        EVList.resize(meshNum+1);
        FEList.resize(meshNum+1);
        EFList.resize(meshNum+1);
        N.resize(meshNum+1);
      }
    
      VList[meshNum]=V;
      FList[meshNum]=F;
      EVList[meshNum]=EV;
      FEList[meshNum]=FE;
      EFList[meshNum]=EF;
    }
    
    void IGL_INLINE set_mesh_colors(const Eigen::MatrixXd& C=Eigen::MatrixXd(),
                                    const int meshNum=0)
    {
      Eigen::MatrixXd meshColors;
      if (C.rows()==0)
        meshColors=default_mesh_color();
      else
        meshColors=C;
      
      if ((VList[meshNum].rows()==FList[meshNum].rows())&&(meshColors.rows()!=VList[meshNum].rows()))  //assume face_based was meant
        data_list[NUMBER_OF_SUBMESHES*meshNum].set_face_based(true);
      data_list[NUMBER_OF_SUBMESHES*meshNum].set_colors(meshColors);
      data_list[NUMBER_OF_SUBMESHES*meshNum].show_faces=true;
      if (edgeVList[meshNum].size()!=0){
        data_list[NUMBER_OF_SUBMESHES*meshNum+EDGE_DIAMOND_MESH].show_faces=false;
        data_list[NUMBER_OF_SUBMESHES*meshNum+EDGE_DIAMOND_MESH].show_lines=false;
      }
      selected_data_index=NUMBER_OF_SUBMESHES*meshNum;
      //CList[meshNum]=C;
    }
    
    void IGL_INLINE set_vertex_data(const Eigen::VectorXd& vertexData,
                                    const double minRange,
                                    const double maxRange,
                                    const int meshNum=0)
    {
      Eigen::MatrixXd C;
      igl::parula(vertexData, minRange,maxRange, C);
      set_mesh_colors(C, meshNum);
    }
    
    void IGL_INLINE set_face_data(const Eigen::VectorXd& faceData,
                                  const double minRange,
                                  const double maxRange,
                                  const int meshNum=0)
    {
      Eigen::MatrixXd C;
      igl::parula(faceData, minRange,maxRange, C);
      set_mesh_colors(C, meshNum);
    }
    
    void IGL_INLINE set_edge_data(const Eigen::VectorXd& edgeData,
                                  const double minRange,
                                  const double maxRange,
                                  const int meshNum=0)
    {
    
      if (edgeVList[meshNum].size()==0){  //allocate
        edge_diamond_mesh(VList[meshNum],FList[meshNum],EVList[meshNum],EFList[meshNum],edgeVList[meshNum],edgeFList[meshNum],edgeFEList[meshNum]);
        data_list[NUMBER_OF_SUBMESHES*meshNum+EDGE_DIAMOND_MESH].clear();
        data_list[NUMBER_OF_SUBMESHES*meshNum+EDGE_DIAMOND_MESH].set_mesh(edgeVList[meshNum],edgeFList[meshNum]);
      }
      
      Eigen::VectorXd edgeFData(edgeFList[meshNum].rows());
      for (int i=0;i<edgeFList[meshNum].rows();i++)
        edgeFData(i)=edgeData(edgeFEList[meshNum](i));
      
      Eigen::MatrixXd C;
      igl::parula(edgeFData, minRange,maxRange, C);
      data_list[NUMBER_OF_SUBMESHES*meshNum+EDGE_DIAMOND_MESH].set_colors(C);
      
      data_list[NUMBER_OF_SUBMESHES*meshNum+EDGE_DIAMOND_MESH].show_faces=false;
      data_list[NUMBER_OF_SUBMESHES*meshNum+EDGE_DIAMOND_MESH].show_lines=false;
      
      selected_data_index=NUMBER_OF_SUBMESHES*meshNum+EDGE_DIAMOND_MESH;
    }
    
    void IGL_INLINE set_selected_faces(const Eigen::VectorXi& selectedFaces, const int meshNum=0){
      Eigen::MatrixXd CMesh=directional::DirectionalViewer::default_mesh_color().replicate(FList[meshNum].rows(),1);
      for (int i=0;i<selectedFaces.size();i++)
        CMesh.row(selectedFaces(i))=selected_face_color();
      set_mesh_colors(CMesh,meshNum);
      
      //coloring field
      Eigen::MatrixXd glyphColors=directional::DirectionalViewer::default_glyph_color().replicate(FList[meshNum].rows(),N[meshNum]);
      for (int i=0;i<selectedFaces.rows();i++)
        glyphColors.row(selectedFaces(i))=directional::DirectionalViewer::selected_face_glyph_color().replicate(1,N[meshNum]);
      
      set_field_colors(glyphColors,meshNum);
    }
    
    void IGL_INLINE set_selected_vector(const int selectedFace, const int selectedVector, const int meshNum=0)
    {
      Eigen::MatrixXd glyphColors=directional::DirectionalViewer::default_glyph_color().replicate(FList[meshNum].rows(),N[meshNum]);
      glyphColors.row(selectedFace)=directional::DirectionalViewer::selected_face_glyph_color().replicate(1,N[meshNum]);
      glyphColors.block(selectedFace,3*selectedVector,1,3)=directional::DirectionalViewer::selected_vector_glyph_color();
      set_field_colors(glyphColors, meshNum);
    }
    
    void IGL_INLINE set_field(const Eigen::MatrixXd& rawField,
                              const Eigen::MatrixXd& C=Eigen::MatrixXd(),
                              const int meshNum=0,
                              const double sizeRatio = 0.9,
                              const int sparsity=0)
    
    {
      Eigen::MatrixXd fieldColors=C;
      if (C.rows()==0)
        fieldColors=default_glyph_color();
      
      Eigen::MatrixXd VField, CField;
      Eigen::MatrixXi FField;
      N[meshNum]=rawField.cols()/3;
      directional::glyph_lines_mesh(VList[meshNum], FList[meshNum], EFList[meshNum], rawField, fieldColors, VField, FField, CField,sizeRatio, sparsity);
      data_list[NUMBER_OF_SUBMESHES*meshNum+FIELD_MESH].clear();
      data_list[NUMBER_OF_SUBMESHES*meshNum+FIELD_MESH].set_mesh(VField,FField);
      data_list[NUMBER_OF_SUBMESHES*meshNum+FIELD_MESH].set_colors(CField);
      data_list[NUMBER_OF_SUBMESHES*meshNum+FIELD_MESH].show_lines=false;
    }
    
    void IGL_INLINE set_field_colors(const Eigen::MatrixXd& C=Eigen::MatrixXd(),
                                     const int meshNum=0)
    {
      Eigen::MatrixXd fieldColors=C;
      if (C.rows()==0)
        fieldColors=default_glyph_color();
      
      Eigen::MatrixXd CField;
      directional::glyph_lines_mesh(FList[meshNum], N[meshNum], fieldColors, CField);
      data_list[NUMBER_OF_SUBMESHES*meshNum+FIELD_MESH].set_colors(CField);
    }
    
    
    void IGL_INLINE set_singularities(const Eigen::VectorXi& singVertices,
                                      const Eigen::VectorXi& singIndices,
                                      const int meshNum=0,
                                      const double radiusRatio=1.25)
    {
      Eigen::MatrixXd VSings, CSings;
      Eigen::MatrixXi FSings;
      directional::singularity_spheres(VList[meshNum], FList[meshNum], N[meshNum], singVertices, singIndices, default_singularity_colors(N[meshNum]), VSings, FSings, CSings, radiusRatio);
      data_list[NUMBER_OF_SUBMESHES*meshNum+SING_MESH].clear();
      data_list[NUMBER_OF_SUBMESHES*meshNum+SING_MESH].set_mesh(VSings,FSings);
      data_list[NUMBER_OF_SUBMESHES*meshNum+SING_MESH].set_colors(CSings);
      data_list[NUMBER_OF_SUBMESHES*meshNum+SING_MESH].show_lines=false;
      
    }
    
    void IGL_INLINE set_seams(const Eigen::MatrixXi& EV,
                              const Eigen::MatrixXi& FE,
                              const Eigen::MatrixXi& EF,
                              const Eigen::VectorXi& combedMatching,
                              const int meshNum=0,
                              const double widthRatio = 0.05)
    {
      
      
      Eigen::MatrixXd VSeams, CSeams;
      Eigen::MatrixXi FSeams;
      
      Eigen::MatrixXi hlHalfedges = Eigen::MatrixXi::Constant(FList[meshNum].rows(),3,-1);
      
      //figuring out the highlighted halfedges
      for (int i=0;i<combedMatching.size();i++){
        if (combedMatching(i)==0)
          continue;
        
        int inFaceIndex=-1;
        
        if (EF(i,0)!=-1){
          for (int j=0;j<3;j++)
              if (FE(EF(i,0),j)==i)
                inFaceIndex=j;
          hlHalfedges(EF(i,0), inFaceIndex)=0;
        }
        inFaceIndex=-1;
        if (EF(i,1)!=-1){
          for (int j=0;j<3;j++)
            if (FE(EF(i,1),j)==i)
              inFaceIndex=j;
          hlHalfedges(EF(i,1), inFaceIndex)=0;
        }
      }
      
      directional::halfedge_highlights(VList[meshNum], FList[meshNum], hlHalfedges, default_seam_color(),VSeams,FSeams,CSeams, widthRatio, 1e-4);

      data_list[NUMBER_OF_SUBMESHES*meshNum+SEAMS_MESH].clear();
      data_list[NUMBER_OF_SUBMESHES*meshNum+SEAMS_MESH].set_mesh(VSeams, FSeams);
      data_list[NUMBER_OF_SUBMESHES*meshNum+SEAMS_MESH].set_colors(CSeams);
      data_list[NUMBER_OF_SUBMESHES*meshNum+SEAMS_MESH].show_lines = false;
    }
    
    void IGL_INLINE set_streamlines(const Eigen::MatrixXd& P1,
                                    const Eigen::MatrixXd& P2,
                                    const Eigen::MatrixXd& C,
                                    const int meshNum=0,
                                    const double width=0.0005)
    {
      Eigen::MatrixXd VStream, CStream;
      Eigen::MatrixXi FStream;
      directional::line_cylinders(P1,P2, width, C, 4, VStream, FStream, CStream);
      
      data_list[NUMBER_OF_SUBMESHES*meshNum+STREAMLINE_MESH].clear();
      data_list[NUMBER_OF_SUBMESHES*meshNum+STREAMLINE_MESH].set_mesh(VStream, FStream);
      data_list[NUMBER_OF_SUBMESHES*meshNum+STREAMLINE_MESH].set_colors(CStream);
      data_list[NUMBER_OF_SUBMESHES*meshNum+STREAMLINE_MESH].show_lines = false;
    }
    
    void IGL_INLINE set_isolines(const Eigen::MatrixXd& cutV,
                                 const Eigen::MatrixXi& cutF,
                                 const Eigen::MatrixXd& vertexFunction,
                                 const int meshNum=0,
                                 const double sizeRatio=0.1)
    {
      
      
      Eigen::MatrixXd isoV, isoN;
      Eigen::MatrixXi isoE;
      Eigen::VectorXi funcNum;
      
      directional::branched_isolines(cutV, cutF, vertexFunction, isoV, isoE, isoN, funcNum);
      
      double l = sizeRatio*igl::avg_edge_length(cutV, cutF);
      
      Eigen::MatrixXd VIso, CIso;
      Eigen::MatrixXi FIso;
      
      Eigen::MatrixXd funcColors = isoline_colors();
      Eigen::MatrixXd CFunc;
      CFunc.resize(funcNum.size(),3);
      for (int i=0;i<funcNum.size();i++)
        CFunc.row(i)=funcColors.row(funcNum(i));
      
      Eigen::MatrixXd P1, P2;
      P1.conservativeResize(isoE.rows(),3);
      P2.conservativeResize(isoE.rows(),3);
      for (int j=0;j<isoE.rows();j++){
        P1.row(j)=isoV.row(isoE(j,0));
        P2.row(j)=isoV.row(isoE(j,1));
      }

      directional::bar_chains(isoV,isoE,isoN,l,l/2000,CFunc, VIso, FIso, CIso);
      
      data_list[NUMBER_OF_SUBMESHES*meshNum+ISOLINES_MESH].clear();
      data_list[NUMBER_OF_SUBMESHES*meshNum+ISOLINES_MESH].set_mesh(VIso, FIso);
      data_list[NUMBER_OF_SUBMESHES*meshNum+ISOLINES_MESH].set_colors(CIso);
      data_list[NUMBER_OF_SUBMESHES*meshNum+ISOLINES_MESH].show_lines = false;
    }
    
    void IGL_INLINE set_uv(const Eigen::MatrixXd UV,
                           const int meshNum=0)
    {
      data_list[NUMBER_OF_SUBMESHES*meshNum].set_uv(UV);
    }
    
    void IGL_INLINE set_texture(const Eigen::Matrix<unsigned char,Eigen::Dynamic,Eigen::Dynamic>& R,
                                const Eigen::Matrix<unsigned char,Eigen::Dynamic,Eigen::Dynamic>& G,
                                const Eigen::Matrix<unsigned char,Eigen::Dynamic,Eigen::Dynamic>& B,
                                const int meshNum=0)
    {
      data_list[NUMBER_OF_SUBMESHES*meshNum].set_texture(R,G,B);
    }
    
    void IGL_INLINE set_active(const bool active, const int meshNum=0){
      for (int i=NUMBER_OF_SUBMESHES*meshNum;i<NUMBER_OF_SUBMESHES*meshNum+NUMBER_OF_SUBMESHES;i++)
        data_list[i].show_faces=active;
    }
    
    void IGL_INLINE toggle_mesh(const bool active, const int meshNum=0){
      data_list[NUMBER_OF_SUBMESHES*meshNum].show_faces=active;
    }
    
    void IGL_INLINE toggle_mesh_edges(const bool active, const int meshNum=0){
      data_list[NUMBER_OF_SUBMESHES*meshNum].show_lines=active;
    }
    
    void IGL_INLINE toggle_field(const bool active, const int meshNum=0){
      data_list[NUMBER_OF_SUBMESHES*meshNum+FIELD_MESH].show_faces=active;
    }
    
    void IGL_INLINE toggle_singularities(const bool active, const int meshNum=0){
      data_list[NUMBER_OF_SUBMESHES*meshNum+SING_MESH].show_faces=active;
    }
    
    void IGL_INLINE toggle_seams(const bool active, const int meshNum=0){
      data_list[NUMBER_OF_SUBMESHES*meshNum+SEAMS_MESH].show_faces=active;
    }
    
    void IGL_INLINE toggle_streamlines(const bool active, const int meshNum=0){
      data_list[NUMBER_OF_SUBMESHES*meshNum+STREAMLINE_MESH].show_faces=active;
    }
    
    void IGL_INLINE toggle_isolines(const bool active, const int meshNum=0){
      data_list[NUMBER_OF_SUBMESHES*meshNum+ISOLINES_MESH].show_faces=active;
    }
    
    void IGL_INLINE toggle_texture(const bool active, const int meshNum=0){
      data_list[NUMBER_OF_SUBMESHES*meshNum].show_texture=active;
    }
    
    //disabling the original mesh
    void IGL_INLINE toggle_edge_data(const bool active, const int meshNum=0){
      data_list[NUMBER_OF_SUBMESHES*meshNum+EDGE_DIAMOND_MESH].show_faces=active;
      //data_list[NUMBER_OF_SUBMESHES*meshNum].show_faces=!active;
    }
    
    //static functions for default values
    //Mesh colors
    static Eigen::RowVector3d IGL_INLINE default_mesh_color(){
      return Eigen::RowVector3d::Constant(1.0);
    }
    
    //Color for faces that are selected for editing and constraints
    static Eigen::RowVector3d IGL_INLINE selected_face_color(){
      return Eigen::RowVector3d(0.7,0.2,0.2);
    }
    
    //Glyph colors
    static Eigen::RowVector3d IGL_INLINE default_glyph_color(){
      return Eigen::RowVector3d(0.0,0.2,1.0);
    }
    
    //Glyphs in selected faces
    static Eigen::RowVector3d IGL_INLINE selected_face_glyph_color(){
      return Eigen::RowVector3d(223.0/255.0, 210.0/255.0, 16.0/255.0);
    }
    
    //The selected glyph currently edited from a selected face
    static Eigen::RowVector3d IGL_INLINE selected_vector_glyph_color(){
      return Eigen::RowVector3d(0.0,1.0,0.5);
    }
    
    //Colors by indices in each directional object.
    static Eigen::MatrixXd IGL_INLINE isoline_colors(){
      
      Eigen::Matrix<double, 15,3> glyphPrincipalColors;
      glyphPrincipalColors<< 0.0,0.5,1.0,
      1.0,0.5,0.0,
      0.0,1.0,0.5,
      1.0,0.0,0.5,
      0.5,0.0,1.0,
      0.5,1.0,0.0,
      1.0,0.5,0.5,
      0.5,1.0,0.5,
      0.5,0.5,1.0,
      0.5,1.0,1.0,
      1.0,0.5,1.0,
      1.0,1.0,0.5,
      0.0,1.0,1.0,
      1.0,0.0,1.0,
      1.0,1.0,0.0;
      
      return glyphPrincipalColors;
    }
    
    
    //Colors by indices in each directional object. If the field is combed they will appear coherent across faces.
    static Eigen::MatrixXd IGL_INLINE indexed_glyph_colors(const Eigen::MatrixXd& field, bool signSymmetry=true){
      
      Eigen::Matrix<double, 15,3> glyphPrincipalColors;
      glyphPrincipalColors<< 0.0,0.5,1.0,
      1.0,0.5,0.0,
      0.0,1.0,0.5,
      1.0,0.0,0.5,
      0.5,0.0,1.0,
      0.5,1.0,0.0,
      1.0,0.5,0.5,
      0.5,1.0,0.5,
      0.5,0.5,1.0,
      0.5,1.0,1.0,
      1.0,0.5,1.0,
      1.0,1.0,0.5,
      0.0,1.0,1.0,
      1.0,0.0,1.0,
      1.0,1.0,0.0;
      
      Eigen::MatrixXd fullGlyphColors(field.rows(),field.cols());
      int N = field.cols()/3;
      for (int i=0;i<field.rows();i++)
        for (int j=0;j<N;j++)
          fullGlyphColors.block(i,3*j,1,3)<< (signSymmetry && (N%2==0) ? glyphPrincipalColors.row(j%(N/2)) : glyphPrincipalColors.row(j));
      
      return fullGlyphColors;
    }
    
    //Jet-based singularity colors
    static Eigen::MatrixXd IGL_INLINE default_singularity_colors(const int N){
      Eigen::MatrixXd fullColors;
      Eigen::VectorXd NList(2*N);
      for (int i=0;i<N;i++){
        NList(i)=-N+i;
        NList(N+i)=i+1;
      }
      igl::jet(-NList,true,fullColors);
      return fullColors;
    }
    
    //Colors for emphasized edges, mostly seams and cuts
    static Eigen::RowVector3d IGL_INLINE default_seam_color(){
      return Eigen::RowVector3d(0.0,0.0,0.0);
    }
    
    static Eigen::Matrix<unsigned char,Eigen::Dynamic,Eigen::Dynamic> IGL_INLINE default_texture(){
      Eigen::Matrix<unsigned char,Eigen::Dynamic,Eigen::Dynamic> texture_R,texture_G,texture_B;
      unsigned size = 128;
      unsigned size2 = size/2;
      unsigned lineWidth = 5;
      texture_B.setConstant(size, size, 0);
      texture_G.setConstant(size, size, 0);
      texture_R.setConstant(size, size, 0);
      for (unsigned i=0; i<size; ++i)
        for (unsigned j=size2-lineWidth; j<=size2+lineWidth; ++j)
          texture_B(i,j) = texture_G(i,j) = texture_R(i,j) = 255;
      for (unsigned i=size2-lineWidth; i<=size2+lineWidth; ++i)
        for (unsigned j=0; j<size; ++j)
          texture_B(i,j) = texture_G(i,j) = texture_R(i,j) = 255;
      
      Eigen::Matrix<unsigned char,Eigen::Dynamic,Eigen::Dynamic> fullTexMat(size*3,size);
      fullTexMat<<texture_R, texture_G, texture_B;
      return fullTexMat;
    }
    
  };  //of DirectionalViewer class
  
  }


#endif
