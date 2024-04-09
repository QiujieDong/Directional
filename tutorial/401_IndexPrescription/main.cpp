#include <iostream>
#include <Eigen/Core>
#include <directional/readOBJ.h>
#include <directional/TriMesh.h>
#include <directional/CartesianField.h>
#include <directional/index_prescription.h>
#include <directional/rotation_to_raw.h>
#include <directional/write_raw_field.h>
#include <directional/read_singularities.h>
#include <directional/directional_viewer.h>


directional::TriMesh mesh;
directional::IntrinsicFaceTangentBundle ftb;
directional::CartesianField field;
Eigen::VectorXi cycleIndices, presSingVertices, presSingIndices;
std::vector<Eigen::VectorXi> cycleFaces;
Eigen::SimplicialLDLT<Eigen::SparseMatrix<double> > ldltSolver;
int currCycle=0;

directional::DirectionalViewer viewer;

int N = 2;

bool drag = false;
bool _select = false;

double globalRotation=0;

void update_field()
{
  using namespace Eigen;
  VectorXd rotationAngles;
  double linfError;
  
  int sum = round(cycleIndices.head(cycleIndices.size() - mesh.numGenerators).sum());
  if (mesh.eulerChar*N != sum)
  {
    std::cout << "Warning: All non-generator singularities should add up to N * the Euler characteristic."<<std::endl;
    std::cout << "Total indices: " << sum <<"/"<<N<< std::endl;
    std::cout << "Expected: " << mesh.eulerChar*N<<"/"<<N<< std::endl;
  }
  
  directional::index_prescription(cycleIndices, N,globalRotation, ldltSolver, field, rotationAngles, linfError);
  std::cout<<"Index prescription linfError: "<<linfError<<std::endl;
  
  viewer.set_field(field);
  viewer.highlight_faces(cycleFaces[currCycle]);

}

void update_singularities()
{
  Eigen::VectorXi singVertices, singIndices;
  std::vector<int> singVerticesList, singIndicesList;
  for (int i=0;i<field.tb->local2Cycle.rows();i++)
    if (cycleIndices(field.tb->local2Cycle(i))){
      singVerticesList.push_back(i);
      singIndicesList.push_back(cycleIndices(field.tb->local2Cycle(i)));
    }
  
  singVertices.resize(singVerticesList.size());
  singIndices.resize(singIndicesList.size());
  for (int i=0;i<singVerticesList.size();i++){
    singVertices(i)=singVerticesList[i];
    singIndices(i)=singIndicesList[i];
  }
  
  viewer.set_singularities(singVertices, singIndices);
}


/*bool mouse_down(igl::opengl::glfw::Viewer& _viewer, int key, int modifiers)
{
  if ((key != 0)||(!_select))
    return false;
  int fid;
  Eigen::Vector3d bc;
  
  // Cast a ray in the view direction starting from the mouse position
  double x = viewer.current_mouse_x;
  double y = viewer.core().viewport(3) - viewer.current_mouse_y;
  if (igl::unproject_onto_mesh(Eigen::Vector2f(x, y), viewer.core().view,
                               viewer.core().proj, viewer.core().viewport, mesh.V, mesh.F, fid, bc))
  {
    Eigen::Vector3d::Index maxCol;
    bc.maxCoeff(&maxCol);
    int currVertex=mesh.F(fid, maxCol);
    currCycle=field.tb->local2Cycle(currVertex);
    viewer.set_selected_faces(cycleFaces[currCycle]);
    return true;
  }
  return false;
};*/

void callbackFunc() {

    ImGui::PushItemWidth(100); // Make ui elements 100 pixels wide,
    ImGui::Text("Prescribed cycle index: %d", cycleIndices(currCycle));
    ImGui::SameLine();
    if (ImGui::Button("+")) {
        cycleIndices(currCycle)++;
        update_field();
        update_singularities();
    }
    ImGui::SameLine();
    if (ImGui::Button("-")) {
        cycleIndices(currCycle)--;
        update_field();
        update_singularities();
    }
    if (ImGui::Button("Global Rotation")) {
        globalRotation += directional::PI / 10;
        update_field();
        update_singularities();
    }

    if (ImGui::Button("Change boundary cycle")){
        if (mesh.boundaryLoops.size())
        {
            //Loop through the boundary cycles.
            if (currCycle >= field.tb->cycles.rows()-mesh.boundaryLoops.size()-mesh.numGenerators && currCycle < field.tb->cycles.rows()-mesh.numGenerators-1)
                currCycle++;
            else
                currCycle = field.tb->cycles.rows()-mesh.boundaryLoops.size()-mesh.numGenerators;
            viewer.highlight_faces(cycleFaces[currCycle]);
        }
    }

    if (ImGui::Button("Change generator cycle")){
        if (mesh.numGenerators)
        {
            //Loop through the generators cycles.
            if (currCycle >= field.tb->cycles.rows() - mesh.numGenerators && currCycle < field.tb->cycles.rows() - 1)
                currCycle++;
            else
                currCycle = field.tb->cycles.rows() - mesh.numGenerators;
            viewer.highlight_faces(cycleFaces[currCycle]);
        }
    }

    if (ImGui::Button("Save field")){
        if (directional::write_raw_field(TUTORIAL_DATA_PATH "/index-prescription.rawfield", field))
            std::cout << "Saved raw field" << std::endl;
        else
            std::cout << "Unable to save raw field. Error: " << errno << std::endl;
    }

    ImGui::PopItemWidth();
}


int main()
{
    
  directional::readOBJ(TUTORIAL_DATA_PATH "/fertility.obj",mesh);
  ftb.init(mesh);
  field.init(ftb, directional::fieldTypeEnum::RAW_FIELD, N);

  cycleIndices=Eigen::VectorXi::Constant(field.tb->cycles.rows(),0);
  
  //loading singularities
  directional::read_singularities(TUTORIAL_DATA_PATH "/fertility.sings", N,presSingVertices,presSingIndices);
  field.set_singularities(presSingVertices,presSingIndices);
  
  /*std::cout<<"Euler characteristic: "<<mesh.eulerChar<<std::endl;
  std::cout<<"#generators: "<<mesh.numGenerators<<std::endl;
  std::cout<<"#boundaries: "<<mesh.boundaryLoops.size()<<std::endl;*/
  
  //collecting cycle faces for visualization
  std::vector<std::vector<int> > cycleFaceList(field.tb->cycles.rows());
  for (int k=0; k<field.tb->cycles.outerSize(); ++k){
    for (Eigen::SparseMatrix<double>::InnerIterator it(field.tb->cycles,k); it; ++it){
      int f1=mesh.EF(mesh.innerEdges(it.col()),0);
      int f2=mesh.EF(mesh.innerEdges(it.col()),1);
      if (f1!=-1)
        cycleFaceList[it.row()].push_back(f1);
      if (f2!=-1)
        cycleFaceList[it.row()].push_back(f2);
    }
  }
  
  for (int i=0;i<presSingVertices.size();i++)
    cycleIndices(field.tb->local2Cycle(presSingVertices(i)))=presSingIndices(i);

  cycleFaces.resize(field.tb->cycles.rows());
  for (int i=0;i<cycleFaceList.size();i++)
    cycleFaces[i] = Eigen::Map<Eigen::VectorXi, Eigen::Unaligned>(cycleFaceList[i].data(), cycleFaceList[i].size());
  
  //triangle mesh setup
  viewer.init();
  viewer.set_mesh(mesh);
  viewer.set_field(field);
  viewer.highlight_faces(cycleFaces[currCycle]);
  update_field();
  update_singularities();
  viewer.set_callback(callbackFunc);
  viewer.launch();
}
