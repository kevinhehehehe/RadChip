
#include "MD5Model.h"
#include "MD5ModelLoaderPCH.h"
#include "Helpers.h"
#include "Texture.h"

extern mat4 Projection;
extern mat4 View;
extern mat4 LightView;
extern mat4 LightProjection;
extern mat4 ScaleBias;

MD5Model::MD5Model()
: m_iMD5Version(-1)
, m_iNumJoints(0)
, m_iNumMeshes(0)
, m_bHasAnimation(false)
{
	adjustM = mat4(1.0);
}

MD5Model::~MD5Model()
{
	for (int i = 0; i < m_Meshes.size(); i++){
		delete m_Meshes[i];
	}
}

bool MD5Model::LoadModel( const std::string &filename )
{
    if ( !fs::exists(filename) )
    {
        std::cerr << "MD5Model::LoadModel: Failed to find file: " << filename << std::endl;
        return false;
    }
    
    fs::path filePath = filename;
    // store the parent path used for loading images relative to this file.
    fs::path parent_path = filePath.parent_path();

    std::string param;
    std::string junk;   // Read junk from the file

    fs::ifstream file(filename);
    int fileLength = GetFileLength( file );   
    assert( fileLength > 0 );

    m_Joints.clear();
    m_Meshes.clear();
    
    file >> param;

    while ( !file.eof() )
    {
        if ( param == "MD5Version" )
        {
            file >> m_iMD5Version;
            assert( m_iMD5Version == 10 );
        }
        else if ( param == "commandline" )
        {
            IgnoreLine(file, fileLength ); // Ignore the contents of the line
        }
        else if ( param == "numJoints" )
        {
            file >> m_iNumJoints;
            m_Joints.reserve(m_iNumJoints);
        }
        else if ( param == "numMeshes" )
        {
            file >> m_iNumMeshes;
            m_Meshes.reserve(m_iNumMeshes);
        }
        else if ( param == "joints" )
        {
            Joint joint;
            file >> junk; // Read the '{' character
            for ( int i = 0; i < m_iNumJoints; ++i )
            {
                file >> joint.m_Name >> joint.m_ParentID >> junk
                     >> joint.m_Pos.x >> joint.m_Pos.y >> joint.m_Pos.z >> junk >> junk
                     >> joint.m_Orient.x >> joint.m_Orient.y >> joint.m_Orient.z >> junk;
                
                RemoveQuotes( joint.m_Name );
                ComputeQuatW( joint.m_Orient );

                m_Joints.push_back(joint);
                // Ignore everything else on the line up to the end-of-line character.
                IgnoreLine( file, fileLength );
            }
            file >> junk; // Read the '}' character
        }
        else if ( param == "mesh" )
        {
            Mesh* mesh = new Mesh();
            int numVerts, numTris, numWeights;

            file >> junk; // Read the '{' character
            file >> param;
            while ( param != "}" )  // Read until we get to the '}' character
            {
                if ( param == "shader" )
                {
                    file >> mesh->m_Shader;
                    RemoveQuotes( mesh->m_Shader );

                    fs::path shaderPath( mesh->m_Shader );
                    fs::path texturePath;
                    if ( shaderPath.has_parent_path() )
                    {
                        texturePath = shaderPath;
                    }
                    else
                    {
                        texturePath = parent_path / shaderPath;
                    }

                    if ( !texturePath.has_extension() )
                    {
                        texturePath.replace_extension( ".png" );
                    }

                    mesh->m_TexID = SOIL_load_OGL_texture( texturePath.string().c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
			//		std::cout << texturePath.string() << std::endl;

                    file.ignore(fileLength, '\n' ); // Ignore everything else on the line
                }
                else if ( param == "numverts")
                {
                    file >> numVerts;               // Read in the vertices
                    IgnoreLine(file, fileLength);
                    for ( int i = 0; i < numVerts; ++i )
                    {
                        Vertex vert;

                        file >> junk >> junk >> junk                    // vert vertIndex (
                            >> vert.m_Tex0.x >> vert.m_Tex0.y >> junk  //  s t )
                            >> vert.m_StartWeight >> vert.m_WeightCount;

                        IgnoreLine(file, fileLength);

                        mesh->m_Verts.push_back(vert);
                        mesh->m_Tex2DBuffer.push_back(vert.m_Tex0);
                    }  
                }
                else if ( param == "numtris" )
                {
                    file >> numTris;
                    IgnoreLine(file, fileLength);
                    for ( int i = 0; i < numTris; ++i )
                    {
                        Triangle tri;
                        file >> junk >> junk >> tri.m_Indices[0] >> tri.m_Indices[1] >> tri.m_Indices[2];

                        IgnoreLine( file, fileLength );

                        mesh->m_Tris.push_back(tri);
                        mesh->m_IndexBuffer.push_back( (GLuint)tri.m_Indices[0] );
                        mesh->m_IndexBuffer.push_back( (GLuint)tri.m_Indices[1] );
                        mesh->m_IndexBuffer.push_back( (GLuint)tri.m_Indices[2] );
                    }              
                }
                else if ( param == "numweights" )
                {
                    file >> numWeights;
                    IgnoreLine( file, fileLength );
                    for ( int i = 0; i < numWeights; ++i )
                    {
                        Weight weight;
                        file >> junk >> junk >> weight.m_JointID >> weight.m_Bias >> junk
                            >> weight.m_Pos.x >> weight.m_Pos.y >> weight.m_Pos.z >> junk;

                        IgnoreLine( file, fileLength );
                        mesh->m_Weights.push_back(weight);
                    }
                }
                else
                {
                    IgnoreLine(file, fileLength);
                }
            
                file >> param;
            }

			mesh->vao.generate();
            PrepareMesh(*mesh);
            PrepareNormals(*mesh);
			mesh->vao.setDrawMode(GL_TRIANGLES, mesh->m_IndexBuffer.size(), GL_UNSIGNED_INT, &(mesh->m_IndexBuffer[0]));

            m_Meshes.push_back(mesh);  /////////////////////////////////////////////////////////////////////// probably cause problem

        }

        file >> param;
    }

    assert( m_Joints.size() == m_iNumJoints );
    assert( m_Meshes.size() == m_iNumMeshes );

    return true;
}

bool MD5Model::LoadAnim( const std::string& filename )
{
    if ( m_Animation.LoadAnimation( filename ) )
    {
        // Check to make sure the animation is appropriate for this model
        m_bHasAnimation = CheckAnimation( m_Animation );
    }

    return m_bHasAnimation;
}

bool MD5Model::CheckAnimation( const MD5Animation& animation ) const
{
    if ( m_iNumJoints != animation.GetNumJoints() )
    {
        return false;
    }

    // Check to make sure the joints match up
    for ( unsigned int i = 0; i < m_Joints.size(); ++i )
    {
        const Joint& meshJoint = m_Joints[i];
        const MD5Animation::JointInfo& animJoint = animation.GetJointInfo( i );

        if ( meshJoint.m_Name != animJoint.m_Name || 
             meshJoint.m_ParentID != animJoint.m_ParentID )
        {
            return false;
        }
    }

    return true;
}


// Compute the position of the vertices in object local space
// in the skeleton's bind pose
bool MD5Model::PrepareMesh( Mesh& mesh )
{
    mesh.m_PositionBuffer.clear();
    mesh.m_Tex2DBuffer.clear();

    // Compute vertex positions
    for ( unsigned int i = 0; i < mesh.m_Verts.size(); ++i )
    {
        glm::vec3 finalPos(0);
        Vertex& vert = mesh.m_Verts[i];

        vert.m_Pos = glm::vec3(0);
        vert.m_Normal = glm::vec3(0);

        // Sum the position of the weights
        for ( int j = 0; j < vert.m_WeightCount; ++j )
        {
            Weight& weight = mesh.m_Weights[vert.m_StartWeight + j];
            Joint& joint = m_Joints[weight.m_JointID];
            
            // Convert the weight position from Joint local space to object space
            glm::vec3 rotPos = joint.m_Orient * weight.m_Pos;

            vert.m_Pos += ( joint.m_Pos + rotPos ) * weight.m_Bias;
        }

        mesh.m_PositionBuffer.push_back(vert.m_Pos);
        mesh.m_Tex2DBuffer.push_back(vert.m_Tex0);
    }

	//mesh.vao.addAttrib(GL_ARRAY_BUFFER, 3 * mesh.m_PositionBuffer.size() * sizeof(float), &(mesh.m_PositionBuffer[0]), GL_STATIC_DRAW, 0, 3, GL_FLOAT, GL_FALSE, 0, (GLubyte*)NULL);
	mesh.vao.addAttrib(GL_ARRAY_BUFFER, 2 * mesh.m_Tex2DBuffer.size() * sizeof(float), &(mesh.m_Tex2DBuffer[0]), GL_DYNAMIC_DRAW, 2, 2, GL_FLOAT, GL_FALSE, 0, (GLubyte*)NULL);

    return true;
}

bool MD5Model::PrepareMesh( Mesh& mesh, const MD5Animation::FrameSkeleton& skel )
{
    for ( unsigned int i = 0; i < mesh.m_Verts.size(); ++i )
    {
        const Vertex& vert = mesh.m_Verts[i];
        glm::vec3& pos = mesh.m_PositionBuffer[i];
        glm::vec3& normal = mesh.m_NormalBuffer[i];

        pos = glm::vec3(0);
        normal = glm::vec3(0);

        for ( int j = 0; j < vert.m_WeightCount; ++j )
        {
            const Weight& weight = mesh.m_Weights[vert.m_StartWeight + j];
            const MD5Animation::SkeletonJoint& joint = skel.m_Joints[weight.m_JointID];

            glm::vec3 rotPos = joint.m_Orient * weight.m_Pos;
            pos += ( joint.m_Pos + rotPos ) * weight.m_Bias;

            normal += ( joint.m_Orient * vert.m_Normal ) * weight.m_Bias;
        }
    }
	mesh.vao.addAttrib(GL_ARRAY_BUFFER, 3 * mesh.m_PositionBuffer.size() * sizeof(float), &(mesh.m_PositionBuffer[0]), GL_DYNAMIC_DRAW, 0, 3, GL_FLOAT, GL_FALSE, 0, (GLubyte*)NULL);
	mesh.vao.addAttrib(GL_ARRAY_BUFFER, 3 * mesh.m_NormalBuffer.size() * sizeof(float), &(mesh.m_NormalBuffer[0]), GL_DYNAMIC_DRAW, 1, 3, GL_FLOAT, GL_FALSE, 0, (GLubyte*)NULL);
    return true;
}


// Compute the vertex normals in the Mesh's bind pose
bool MD5Model::PrepareNormals( Mesh& mesh )
{
    mesh.m_NormalBuffer.clear();

    // Loop through all triangles and calculate the normal of each triangle
    for ( unsigned int i = 0; i < mesh.m_Tris.size(); ++i )
    {
        glm::vec3 v0 = mesh.m_Verts[ mesh.m_Tris[i].m_Indices[0] ].m_Pos;
        glm::vec3 v1 = mesh.m_Verts[ mesh.m_Tris[i].m_Indices[1] ].m_Pos;
        glm::vec3 v2 = mesh.m_Verts[ mesh.m_Tris[i].m_Indices[2] ].m_Pos;

        glm::vec3 normal = glm::cross( v1 - v0, v2 - v0 ); //changed from original

        mesh.m_Verts[ mesh.m_Tris[i].m_Indices[0] ].m_Normal += normal;
        mesh.m_Verts[ mesh.m_Tris[i].m_Indices[1] ].m_Normal += normal;
        mesh.m_Verts[ mesh.m_Tris[i].m_Indices[2] ].m_Normal += normal;
    }

    // Now normalize all the normals
    for ( unsigned int i = 0; i < mesh.m_Verts.size(); ++i )
    {
        Vertex& vert = mesh.m_Verts[i];

        glm::vec3 normal = glm::normalize( vert.m_Normal );
		mesh.m_NormalBuffer.push_back(normal);

        // Reset the normal to calculate the bind-pose normal in joint space
        vert.m_Normal = glm::vec3(0);

        // Put the bind-pose normal into joint-local space
        // so the animated normal can be computed faster later
        for ( int j = 0; j < vert.m_WeightCount; ++j )
        {
            const Weight& weight = mesh.m_Weights[vert.m_StartWeight + j];
            const Joint& joint = m_Joints[weight.m_JointID];
            vert.m_Normal += ( normal * joint.m_Orient ) * weight.m_Bias;
        }
    }

	mesh.vao.addAttrib(GL_ARRAY_BUFFER, 3 * mesh.m_NormalBuffer.size() * sizeof(float), &(mesh.m_NormalBuffer[0]), GL_DYNAMIC_DRAW, 1, 3, GL_FLOAT, GL_FALSE, 0, (GLubyte*)NULL);
    return true;
}

void MD5Model::Update( float fDeltaTime )
{
    if ( m_bHasAnimation )
    {
        m_Animation.Update(fDeltaTime);
        const MD5Animation::FrameSkeleton& skeleton = m_Animation.GetSkeleton();

        for ( unsigned int i = 0; i < m_Meshes.size(); ++i )
        {
            PrepareMesh( *(m_Meshes[i]), skeleton );
        }
    }
}

void MD5Model::setShader(GLSLProgram* shader)
{
	this->shader = shader;
	uniformLoc.push_back(shader->getUniformLoc("Projection"));
	uniformLoc.push_back(shader->getUniformLoc("View"));
	uniformLoc.push_back(shader->getUniformLoc("Model"));
	uniformLoc.push_back(shader->getUniformLoc("TexID"));
	uniformLoc.push_back(shader->getUniformLoc("material.Shininess"));
	uniformLoc.push_back(shader->getUniformLoc("material.ReflectFactor"));
	uniformLoc.push_back(shader->getUniformLoc("shadowMap"));
	uniformLoc.push_back(shader->getUniformLoc("LightView"));
	uniformLoc.push_back(shader->getUniformLoc("LightProjection"));
	uniformLoc.push_back(shader->getUniformLoc("ScaleBias"));
}


void MD5Model::draw()
{
	shader->use();
	shader->setUniform(uniformLoc[0],Projection);
	shader->setUniform(uniformLoc[1], View);
	shader->setUniform(uniformLoc[2], getModelM()*getRotation()*adjustM);
	shader->setUniform(uniformLoc[3], 0);
	shader->setUniform(uniformLoc[4], Shininess);
	shader->setUniform(uniformLoc[5], ReflectFactor);
	shader->setUniform(uniformLoc[6], shadowTex);
	shader->setUniform(uniformLoc[7], LightView);
	shader->setUniform(uniformLoc[8], LightProjection);
	shader->setUniform(uniformLoc[9], ScaleBias);
    // Render the meshes
    for ( unsigned int i = 0; i < m_Meshes.size(); ++i )
    {
        RenderMesh( *(m_Meshes[i]) );
    }
	glUseProgram(0);
}

void MD5Model::draw(mat4& projection, mat4& view)
{
	shader->use();
	shader->setUniform(uniformLoc[0], projection);
	shader->setUniform(uniformLoc[1], view);
	shader->setUniform(uniformLoc[2], getModelM()*getRotation()*adjustM);
	shader->setUniform(uniformLoc[3], 0);
	shader->setUniform(uniformLoc[4], Shininess);
	shader->setUniform(uniformLoc[5], ReflectFactor);
	shader->setUniform(uniformLoc[6], shadowTex);
	shader->setUniform(uniformLoc[7], LightView);
	shader->setUniform(uniformLoc[8], LightProjection);
	shader->setUniform(uniformLoc[9], ScaleBias);
	// Render the meshes
	for (unsigned int i = 0; i < m_Meshes.size(); ++i)
	{
		RenderMesh(*(m_Meshes[i]));
	}
	glUseProgram(0);
}

void MD5Model::RenderMesh( const Mesh& mesh )
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mesh.m_TexID);

	mesh.vao.draw();
}

void MD5Model::RenderNormals(  const Mesh& mesh )
{

    glPushAttrib( GL_ENABLE_BIT );
    glDisable( GL_LIGHTING );

    glColor3f( 1.0f, 1.0f, 0.0f );// Yellow

    glBegin( GL_LINES );
    {
        for ( unsigned int i = 0; i < mesh.m_PositionBuffer.size(); ++i )
        {
            glm::vec3 p0 = mesh.m_PositionBuffer[i];
            glm::vec3 p1 = ( mesh.m_PositionBuffer[i] + mesh.m_NormalBuffer[i] );

            glVertex3fv( glm::value_ptr(p0) );
            glVertex3fv( glm::value_ptr(p1) );
        }
    }
    glEnd();

    glPopAttrib();
}

void MD5Model::RenderSkeleton( const JointList& joints )
{
    glPointSize( 5.0f );
    glColor3f( 1.0f, 0.0f, 0.0f );
    
    glPushAttrib( GL_ENABLE_BIT );

    glDisable(GL_LIGHTING );
    glDisable( GL_DEPTH_TEST );

    // Draw the joint positions
    glBegin( GL_POINTS );
    {
        for ( unsigned int i = 0; i < joints.size(); ++i )
        {
            glVertex3fv( glm::value_ptr(joints[i].m_Pos) );
        }
    }
    glEnd();

    // Draw the bones
    glColor3f( 0.0f, 1.0f, 0.0f );
    glBegin( GL_LINES );
    {
        for ( unsigned int i = 0; i < joints.size(); ++i )
        {
            const Joint& j0 = joints[i];
            if ( j0.m_ParentID != -1 )
            {
                const Joint& j1 = joints[j0.m_ParentID];
                glVertex3fv( glm::value_ptr(j0.m_Pos) );
                glVertex3fv( glm::value_ptr(j1.m_Pos) );
            }
        }
    }
    glEnd();

    glPopAttrib();

}
