#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat4;
using glm::mat3;


void glWindowPos4fMESAemulate(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	GLfloat fx, fy;
	/* Push current matrix mode and viewport attributes. */

	glPushAttrib(GL_TRANSFORM_BIT | GL_VIEWPORT_BIT);
	/* Setup projection parameters. */

	glMatrixMode(GL_PROJECTION);

	glPushMatrix();

	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);

	glPushMatrix();

	glLoadIdentity();

	glDepthRange(z, z);

	glViewport((int)x - 1, (int)y - 1, 2, 2);

	/* Set the raster (window) position. */

	fx = x - (int)x;

	fy = y - (int)y;

	glRasterPos4f(fx, fy, 0.0, w);

	/* Restore matrices, viewport and matrix mode. */

	glPopMatrix();

	glMatrixMode(GL_PROJECTION);

	glPopMatrix();

	glPopAttrib();

}

void glWindowPos2fMESAemulate(GLfloat x, GLfloat y)
{
	glWindowPos4fMESAemulate(x, y, 0, 1);
}

// (0,0) is the bottom left of the window regardless of size
void RenderString(float x, float y, void *font, const unsigned char* string, vec3 const& rgb)
{
	//char *c;

	glColor3f(rgb.r, rgb.g, rgb.b);
	//glColor4f(0.0f, 0.0f, 1.0f, 1.0f);

	//glVertex2i(x, y);
	//glRasterPos2f(x, y);
	glWindowPos2fMESAemulate(x, y);

	glutBitmapString(font, string);
}

std::string ConvertAddress(unsigned char* string)
{
	int i = 0;
	std::string word;
	while (string[i] != 0 && string[i] != '|')
	{
		if (!((string[i] > 47 && string[i] < 58) || string[i] == 46))
		{
			word = "localhost";
			break;
		}
		word += string[i];
		i++;
	}

	if (i == 0)
	{
		word = "localhost";
	}
	
	return word;
}

class UI_Panel : public Object
{
public:
	UI_Panel(void){
		texBool = 0;
		generate(-0.5, 0.5, -0.5, 0.5);
		setModelM(mat4(1.0));
		vao.generate();
		vao.addAttrib(GL_ARRAY_BUFFER, 8 * sizeof(float), &vertex_positions, GL_STATIC_DRAW, 0, 2, GL_FLOAT, GL_FALSE, 0, (GLubyte*)NULL);
		vao.addAttrib(GL_ARRAY_BUFFER, 8 * sizeof(float), &tex_positions, GL_STATIC_DRAW, 1, 2, GL_FLOAT, GL_FALSE, 0, (GLubyte*)NULL);
		vao.setDrawMode(GL_TRIANGLES, 6, GL_UNSIGNED_INT, &triangle_indices);
	}
	UI_Panel(float negx, float posx, float negy, float posy){
		texBool = 0;
		generate(negx, posx, negy, posy);
		setModelM(mat4(1.0));
		vao.generate();
		vao.addAttrib(GL_ARRAY_BUFFER, 8 * sizeof(float), &vertex_positions, GL_STATIC_DRAW, 0, 2, GL_FLOAT, GL_FALSE, 0, (GLubyte*)NULL);
		vao.addAttrib(GL_ARRAY_BUFFER, 8 * sizeof(float), &tex_positions, GL_STATIC_DRAW, 1, 2, GL_FLOAT, GL_FALSE, 0, (GLubyte*)NULL);
		vao.setDrawMode(GL_TRIANGLES, 6, GL_UNSIGNED_INT, &triangle_indices);
	}
	~UI_Panel(void){}
	void draw(){
		shader->setUniform(uniformLoc[0], View);
		shader->setUniform(uniformLoc[1], Projection);
		shader->setUniform(uniformLoc[2], getModelM());
		shader->setUniform(uniformLoc[3], Projection*View*getModelM());
		shader->setUniform(uniformLoc[4], color);
		shader->setUniform(uniformLoc[5], 0);
		shader->setUniform(uniformLoc[6], texBool);
		if (texBool == 1){
			colorTex->Bind(GL_TEXTURE0);
		}		
		shader->use();
		vao.draw();
		glUseProgram(0);
	}
	void draw(mat4& projection, mat4& view){
		shader->setUniform(uniformLoc[0], view);
		shader->setUniform(uniformLoc[1], projection);
		shader->setUniform(uniformLoc[2], getModelM());
		shader->setUniform(uniformLoc[3], projection*view*getModelM());
		shader->setUniform(uniformLoc[4], color);
		shader->setUniform(uniformLoc[5], 0);
		shader->setUniform(uniformLoc[6], texBool);
		if (texBool == 1){
			colorTex->Bind(GL_TEXTURE0);
		}
		shader->use();
		vao.draw();
		glUseProgram(0);
	}
	void setShader(GLSLProgram* shader){
		this->shader = shader;
		uniformLoc.push_back(shader->getUniformLoc("ViewMatrix"));
		uniformLoc.push_back(shader->getUniformLoc("ProjectionMatrix"));
		uniformLoc.push_back(shader->getUniformLoc("ModelMatrix"));
		uniformLoc.push_back(shader->getUniformLoc("MVP"));
		uniformLoc.push_back(shader->getUniformLoc("color"));
		uniformLoc.push_back(shader->getUniformLoc("colorTex"));
		uniformLoc.push_back(shader->getUniformLoc("textureMeBaby"));
	}
	void setColor(vec3 c){ color = c; }
	
	void loadColorTex(const char* FileName, const char* FileType){
		colorTex = new Texture(GL_TEXTURE_2D, FileName, FileType);
		colorTex->Load();
	}

	void setTex(bool b){
		if (b){
			texBool = 1;
		}
		else{
			texBool = 0;
		}
	}
private:
	void generate(float negx, float posx, float negy, float posy){
		std::array<float, 8> positions = {negx, posy, negx, negy, posx, posy, posx, negy}; 
		std::array<float, 8> texCord = { 0, 1, 0, 0, 1, 1, 1, 0 };
		std::array<int, 6> indices = {0, 1, 2, 1, 3, 2};    
		vertex_positions = positions;
		tex_positions = texCord;
		triangle_indices = indices;
	}
	VAO vao;
	GLSLProgram * shader;
	std::array<float, 8> vertex_positions;
	std::array<float, 8> tex_positions;
	std::array<int, 6> triangle_indices;
	int CubeMapUnit;
	vec3 color;
	vector<int> uniformLoc;
	Texture * colorTex;
	int texBool;
};

class UI
{
public:
	UI(){

		//life bars
		life_back = new UI_Panel(x1_life, x2_life, y1_life, y2_life);
		life_back->setColor(vec3(1.0, 0.0, 0.0));
		life_back->setShader(sdrCtl.getShader("basic_2D"));
		//life_back->loadColorTex("img/UI_TEST.png", "PNG");
		life_back->setModelM(glm::scale(vec3(1.0, 1.0, 1.0))*glm::translate(vec3(0.0f, 0.42f, -1.0f)));

		life_front = new UI_Panel(x1_life, x2_life, y1_life, y2_life);
		life_front->setColor(vec3(0.0, 1.0, 0.0));
		life_front->setShader(sdrCtl.getShader("basic_2D"));
		//life_front->loadColorTex("img/UI_TEST.png", "PNG");
		life_front->setModelM(glm::scale(vec3(1.0, 1.0, 1.0))*glm::translate(vec3(0.0f, 0.42f, -1.0f)));

		life_frame = new UI_Panel(-0.3f, 0.3f, -0.2f, 0.2f);
		life_frame->setColor(vec3(1.0, 0.0, 0.0));
		life_frame->setShader(sdrCtl.getShader("basic_2D"));
		life_frame->loadColorTex("img/UI_elements/healthBAR_2", "PNG");
		life_frame->setTex(true);
		life_frame->setModelM(glm::scale(vec3(1.0, 1.0, 1.0))*glm::translate(vec3(0.0f, 0.42f, -1.0f)));

		//overheat bars
		heat_back = new UI_Panel(x1_heat, x2_heat, y1_heat, -1.0*(y2_heat));
		heat_back->setColor(vec3(1.0, 1.0, 1.0));
		heat_back->setShader(sdrCtl.getShader("basic_2D"));
		//heat_back->loadColorTex("img/UI_TEST.png", "PNG");
		heat_back->setModelM(glm::scale(vec3(1.0, 1.0, 1.0))*glm::translate(vec3(-0.85f, 0.0f, -1.0f)));

		heat_front = new UI_Panel(x1_heat, x2_heat, y1_heat, y2_heat);
		heat_front->setColor(vec3(0.0, 1.0, 0.0));
		heat_front->setShader(sdrCtl.getShader("basic_2D"));
		//heat_front->loadColorTex("img/UI_TEST.png", "PNG");
		heat_front->setModelM(glm::scale(vec3(1.0, 1.0, 1.0))*glm::translate(vec3(-0.85f, 0.0f, -1.0f)));

		heat_frame = new UI_Panel(x1_heat, x2_heat, y1_heat, -1.0*(y2_heat));
		heat_frame->setColor(vec3(1.0, 1.0, 1.0));
		heat_frame->setShader(sdrCtl.getShader("basic_2D"));
		heat_frame->loadColorTex("img/UI_FRAME_VERT.png", "PNG");
		heat_frame->setTex(true);
		heat_frame->setModelM(glm::scale(vec3(1.0, 1.0, 1.0))*glm::translate(vec3(-0.85f, 0.0f, -1.0f)));

		//tower indicators in the top right and left coreners of the screen

		m_tower1 = new UI_Panel(-0.01f, 0.01f, -0.01f, 0.01f);
		m_tower1->setColor(vec3(1.0, 1.0, 1.0));
		m_tower1->setShader(sdrCtl.getShader("basic_2D"));
		m_tower1->loadColorTex("img/UI_elements/minusSign.png", "PNG");
		m_tower1->setTex(true);
		m_tower1->setModelM(glm::scale(vec3(1.0, 1.0, 1.0))*glm::translate(vec3(-0.55f, 0.45f, -1.0f)));

		m_tower2 = new UI_Panel(-0.01f, 0.01f, -0.01f, 0.01f);
		m_tower2->setColor(vec3(1.0, 1.0, 1.0));
		m_tower2->setShader(sdrCtl.getShader("basic_2D"));
		m_tower2->loadColorTex("img/UI_elements/minusSign.png", "PNG");
		m_tower2->setTex(true);
		m_tower2->setModelM(glm::scale(vec3(1.0, 1.0, 1.0))*glm::translate(vec3(-0.60f, 0.45f, -1.0f)));

		m_tower3 = new UI_Panel(-0.01f, 0.01f, -0.01f, 0.01f);
		m_tower3->setColor(vec3(1.0, 1.0, 1.0));
		m_tower3->setShader(sdrCtl.getShader("basic_2D"));
		m_tower3->loadColorTex("img/UI_elements/minusSign.png", "PNG");
		m_tower3->setTex(true);
		m_tower3->setModelM(glm::scale(vec3(1.0, 1.0, 1.0))*glm::translate(vec3(-0.65f, 0.45f, -1.0f)));

		c_tower1 = new UI_Panel(-0.01f, 0.01f, -0.01f, 0.01f);
		c_tower1->setColor(vec3(1.0, 1.0, 1.0));
		c_tower1->setShader(sdrCtl.getShader("basic_2D"));
		c_tower1->loadColorTex("img/UI_elements/plusSign.png", "PNG");
		c_tower1->setTex(true);
		c_tower1->setModelM(glm::scale(vec3(1.0, 1.0, 1.0))*glm::translate(vec3(0.55f, 0.45f, -1.0f)));

		c_tower2 = new UI_Panel(-0.01f, 0.01f, -0.01f, 0.01f);
		c_tower2->setColor(vec3(1.0, 1.0, 1.0));
		c_tower2->setShader(sdrCtl.getShader("basic_2D"));
		c_tower2->loadColorTex("img/UI_elements/plusSign.png", "PNG");
		c_tower2->setTex(true);
		c_tower2->setModelM(glm::scale(vec3(1.0, 1.0, 1.0))*glm::translate(vec3(0.60f, 0.45f, -1.0f)));

		c_tower3 = new UI_Panel(-0.01f, 0.01f, -0.01f, 0.01f);
		c_tower3->setColor(vec3(1.0, 1.0, 1.0));
		c_tower3->setShader(sdrCtl.getShader("basic_2D"));
		c_tower3->loadColorTex("img/UI_elements/plusSign.png", "PNG");
		c_tower3->setTex(true);
		c_tower3->setModelM(glm::scale(vec3(1.0, 1.0, 1.0))*glm::translate(vec3(0.65f, 0.45f, -1.0f)));


	}
	~UI(){
		life_back->		~UI_Panel();
		life_front->	~UI_Panel();
		life_frame->	~UI_Panel();
		heat_back->		~UI_Panel();
		heat_front->	~UI_Panel();
		heat_frame->	~UI_Panel();

		m_tower1->		~UI_Panel();
		m_tower2->	    ~UI_Panel();
		m_tower3->	    ~UI_Panel();
		c_tower1->		~UI_Panel();
		c_tower2->	    ~UI_Panel();
		c_tower3->	    ~UI_Panel();
	}

	int draw(){

		//Status Bars
		life_back->draw();
		life_front->draw();

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//glDepthMask(GL_FALSE);

		life_frame->draw();

		glDisable(GL_BLEND);
		//glDepthMask(GL_TRUE);

		overheatBar(shots, overheat, 2);

		heat_back->draw();
		heat_front->draw();
		heat_frame->draw();

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		switch (m_towers_left){
		case 3:
			m_tower3->draw();
		case 2:
			m_tower2->draw();
		case 1:
			m_tower1->draw();
		default:
			break;
		}

		switch (c_towers_left){
		case 3:
			c_tower3->draw();
		case 2:
			c_tower2->draw();
		case 1:
			c_tower1->draw();
		default:
			break;
		}

		
	
		/*m_tower3->draw();
		m_tower2->draw();
		m_tower1->draw();

		c_tower3->draw();
		c_tower2->draw();
		c_tower1->draw(); */
		

		glDisable(GL_BLEND);
		
		return 0;
	}

	int healthBar(float damage){

		life_front->UI_Panel::~UI_Panel();

		x2_life = x2_life - health_bar_size*damage;

		if (x2_life <= x1_life)
		{
			//x2_life = x1_life;
			x2_life = 0.275f;
		}

		life_front = new UI_Panel(x1_life, x2_life, y1_life, y2_life);
		life_front->setColor(vec3(0.0, 1.0, 0.0));
		life_front->setShader(sdrCtl.getShader("basic_2D"));
		//life_front->loadColorTex("img/UI_TEST.png", "PNG");
		life_front->setModelM(glm::scale(vec3(1.0, 1.0, 1.0))*glm::translate(vec3(0.0f, 0.42f, -1.0f)));

		return 0;

	}

	int overheatBar(int fired, int hot, int style){

		//style will state what class character is meele or range 1 been meele, more cases can be added later
		if (style == 1)
		{
			if (lock == 0)
			{
				heat_back->UI_Panel::~UI_Panel();
				heat_front->UI_Panel::~UI_Panel();
				lock = 1;
			}
		}

		else
		{
			if (fired == 1 && hot == 0)
			{
				heat_front->UI_Panel::~UI_Panel();

				y2_heat = y2_heat + ((heat_bar_size) / 7.0); //# of shots

				if (y2_heat >= 0.2f)
				{
					y2_heat = 0.2f;
					overheat = 1;
				}

				heat_front = new UI_Panel(x1_heat, x2_heat, y1_heat, y2_heat);
				heat_front->setColor(vec3(0.0, 1.0, 0.0));
				heat_front->setShader(sdrCtl.getShader("basic_2D"));
				//heat_front->loadColorTex("img/UI_TEST.png", "PNG");
				heat_front->setModelM(glm::scale(vec3(1.0, 1.0, 1.0))*glm::translate(vec3(-0.85f - x, 0.0f, -1.0f)));



				over_de = clock();
				shots = 0;
			}


			else if (fired == 0 && hot == 0)
			{
				heat_front->UI_Panel::~UI_Panel();

				if (y2_heat > y1_heat && (clock() - over_de) / (float)CLOCKS_PER_SEC > 0.05)
				{
					y2_heat = y2_heat - ((heat_bar_size) / 60.0);
					over_de = clock();
				}

				if (y2_heat <= y1_heat)
				{
					y2_heat = y1_heat;
				}


				heat_front = new UI_Panel(x1_heat, x2_heat, y1_heat, y2_heat);
				heat_front->setColor(vec3(0.0, 1.0, 0.0));
				heat_front->setShader(sdrCtl.getShader("basic_2D"));
				//heat_front->loadColorTex("img/UI_TEST.png", "PNG");
				heat_front->setModelM(glm::scale(vec3(1.0, 1.0, 1.0))*glm::translate(vec3(-0.85f - x, 0.0f, -1.0f)));
			}

			else if (hot == 1)
			{
				heat_front->UI_Panel::~UI_Panel();

				if ((clock() - over_de) / (float)CLOCKS_PER_SEC > 0.05)
				{
					y2_heat = y2_heat - ((heat_bar_size) / 60.0);
					over_de = clock();
				}

				if (y2_heat < y1_heat)
				{
					y2_heat = y1_heat;
					overheat = 0;
				}

				heat_front = new UI_Panel(x1_heat, x2_heat, y1_heat, y2_heat);
				heat_front->setColor(vec3(1.0, 0.0, 0.0));
				heat_front->setShader(sdrCtl.getShader("basic_2D"));
				//heat_front->loadColorTex("img/UI_TEST.png", "PNG");
				heat_front->setModelM(glm::scale(vec3(1.0, 1.0, 1.0))*glm::translate(vec3(-0.85f - x, 0.0f, -1.0f)));

			}
		}

		return 0;
	}



	float getLess_Life(){ return less_life; }
	void setLess_Life(float l){ less_life = l; }

	float getShots(){ return shots; }
	void setShots(float s){ shots = s; }

	void setTowers(char team){
		if (team == 'm'){
			m_towers_left-- ;
		}

		else{
			c_towers_left--;
		}
	}

	void setOffSet(float w){ x += w; }
	void updateUIposition(float w){
		// needs to include the height as well, it affects the scaling 
		y = -0.00067f * w;

		heat_front->setModelM(glm::scale(vec3(1.0, 1.0, 1.0))*glm::translate(vec3(y, 0.0f, -1.0f)));
		heat_back->setModelM(glm::scale(vec3(1.0, 1.0, 1.0))*glm::translate(vec3(y, 0.0f, -1.0f)));
		heat_frame->setModelM(glm::scale(vec3(1.0, 1.0, 1.0))*glm::translate(vec3(y, 0.0f, -1.0f)));

		x = -0.85f - y;

	}

private:

	UI_Panel* life_back;
	UI_Panel* life_front;
	UI_Panel* life_frame;
	UI_Panel* heat_back;
	UI_Panel* heat_front;
	UI_Panel* heat_frame;

	UI_Panel* m_tower1;
	UI_Panel* m_tower2;
	UI_Panel* m_tower3;
	UI_Panel* c_tower1;
	UI_Panel* c_tower2;
	UI_Panel* c_tower3;
	

	float x1_life = -0.27f; //life
	float x2_life = 0.275f;
	float y1_life = -0.035f;
	float y2_life = 0.06f;

	float x1_heat = -0.02f; // overheat bar
	float x2_heat = 0.02f;
	float y1_heat = -0.2f;
	float y2_heat = -0.2f;

	float x = 0;
	float y = 0;

	int lock = 0;

	float less_life = 0;
	float overheat = 0;
	float shots = 0;
	float damage_taken = (float)0.1; //set to default 1/7 of the life bar
	float health_bar_size = x2_life - x1_life;
	float heat_bar_size = (-1)*y2_heat - y1_heat;

	int c_towers_left = 3;
	int m_towers_left = 3;

	time_t over_de; //rate of decay of overheat bar

};


class MainMenu
{
public:
	MainMenu(){
		menu = new UI_Panel(-0.9f, 0.9f, -0.5f, 0.5f);
		menu->setColor(vec3(1.0, 0.0, 0.0));
		menu->setShader(sdrCtl.getShader("basic_2D"));
		menu->loadColorTex("img/back", "PNG");
		menu->setTex(true);
		menu->setModelM(glm::scale(vec3(1.0f, 1.0f, 1.0f))*glm::translate(vec3(0.0f, 0.0, -1.0f)));

		game_name = new UI_Panel(-0.5f, 0.5f, -0.05f, 0.05f);
		game_name->setColor(vec3(1.0, 0.0, 0.0));
		game_name->setShader(sdrCtl.getShader("basic_2D"));
		game_name->loadColorTex("img/team4", "PNG");
		game_name->setTex(true);
		game_name->setModelM(glm::scale(vec3(1.0, 1.0, 1.0))*glm::translate(vec3(0.0f, 0.45f, -1.0f)));

		start_button = new UI_Panel(-0.15f, 0.15f, -0.05f, 0.05f);
		start_button->setColor(vec3(1.0, 0.0, 0.0));
		start_button->setShader(sdrCtl.getShader("basic_2D"));
		start_button->loadColorTex("img/UI_elements/button_stainlessSteel_StartOFF_1.png", "PNG");
		start_button->setTex(true);
		start_button->setModelM(glm::scale(vec3(1.0f, 1.0f, 1.0))*glm::translate(vec3(0.0f, 0.1f, -1.0f)));

		exit_button = new UI_Panel(-0.15f, 0.15f, -0.05f, 0.05f);
		exit_button->setColor(vec3(1.0, 0.0, 0.0));
		exit_button->setShader(sdrCtl.getShader("basic_2D"));
		exit_button->loadColorTex("img/UI_elements/button_stainlessSteel_ExitOFF_1.png", "PNG");
		exit_button->setTex(true);
		exit_button->setModelM(glm::scale(vec3(1.0f, 1.0f, 1.0))*glm::translate(vec3(0.0f, -0.3f, -1.0f)));

		setting_button = new UI_Panel(-0.15f, 0.15f, -0.05f, 0.05f);
		setting_button->setColor(vec3(1.0, 0.0, 0.0));
		setting_button->setShader(sdrCtl.getShader("basic_2D"));
		setting_button->loadColorTex("img/UI_elements/button_stainlessSteel_SettingsOFF.png", "PNG");
		setting_button->setTex(true);
		setting_button->setModelM(glm::scale(vec3(1.0f, 1.0f, 1.0))*glm::translate(vec3(0.0f, -0.1f, -1.0f)));

		selected_start_button = new UI_Panel(-0.15f, 0.15f, -0.05f, 0.05f);
		selected_start_button->setColor(vec3(1.0, 0.0, 0.0));
		selected_start_button->setShader(sdrCtl.getShader("basic_2D"));
		selected_start_button->loadColorTex("img/UI_elements/button_stainlessSteel_StartON_1.png", "PNG");
		selected_start_button->setTex(true);
		selected_start_button->setModelM(glm::scale(vec3(1.0f, 1.0f, 1.0))*glm::translate(vec3(0.0f, 0.1f, -1.0f)));

		selected_end_button = new UI_Panel(-0.15f, 0.15f, -0.05f, 0.05f);
		selected_end_button->setColor(vec3(1.0, 0.0, 0.0));
		selected_end_button->setShader(sdrCtl.getShader("basic_2D"));
		selected_end_button->loadColorTex("img/UI_elements/button_stainlessSteel_ExitON.png", "PNG");
		selected_end_button->setTex(true);
		selected_end_button->setModelM(glm::scale(vec3(1.0f, 1.0f, 1.0))*glm::translate(vec3(0.0f, -0.3f, -1.0f)));

		selected_setting_button = new UI_Panel(-0.15f, 0.15f, -0.05f, 0.05f);
		selected_setting_button->setColor(vec3(1.0, 0.0, 0.0));
		selected_setting_button->setShader(sdrCtl.getShader("basic_2D"));
		selected_setting_button->loadColorTex("img/UI_elements/button_stainlessSteel_SettingsON.png", "PNG");
		selected_setting_button->setTex(true);
		selected_setting_button->setModelM(glm::scale(vec3(1.0f, 1.0f, 1.0))*glm::translate(vec3(0.0f, -0.1f, -1.0f)));

		address_bar = new UI_Panel(-0.2f, 0.2f, -0.1f, 0.1f);
		address_bar->setColor(vec3(1.0, 0.0, 0.0));
		address_bar->setShader(sdrCtl.getShader("basic_2D"));
		address_bar->loadColorTex("img/UI_elements/IP_adressBAR", "PNG");
		address_bar->setTex(true);
		address_bar->setModelM(glm::scale(vec3(1.0f, 1.0f, 1.0f))*glm::translate(vec3(0.0f, 0.3f, -1.0f)));
	}
	~MainMenu(){
		menu->					  ~UI_Panel();
		game_name->				  ~UI_Panel();
		start_button->			  ~UI_Panel();
		exit_button->			  ~UI_Panel();
		setting_button->          ~UI_Panel();
		selected_start_button->	  ~UI_Panel();
		selected_end_button->	  ~UI_Panel();
		selected_setting_button-> ~UI_Panel();
		address_bar->			  ~UI_Panel();

	}

	int draw(){

		glDisable(GL_DEPTH_TEST);

		menu->draw();
		game_name->draw();
		start_button->draw();
		exit_button->draw();
		setting_button->draw();
		address_bar->draw();

		if (drawStartHighlight){
			selected_start_button->draw();
		}

		if (drawEndHighlight){
			selected_end_button->draw();
		}

		if (drawSettingHighlight){
			selected_setting_button->draw();
		}

		glEnable(GL_DEPTH_TEST);

		return 0;
	}

	int checkHighlight(float x, float y){

		//Check the x bounds first cause all buttons are the same width
		if ((x > 0.42) && (x < 0.57)){

			//Spot for first button
			if ((y > 0.35) && (y < 0.47)){
				drawStartHighlight = true;
				drawEndHighlight = false;
				drawSettingHighlight = false;

				return 1;
				//cout << "Start Monica!" << endl;
			}

			//Spot for second button
			else if ((y > 0.53) && (y < 0.64)){
				drawSettingHighlight = true;
				drawEndHighlight = false;
				drawStartHighlight = false;

				return 1;
				//cout << "End Monica!" << endl;
			}

			//Spot for third button
			else if ((y > 0.71) && (y < 0.81))
			{
				drawEndHighlight = true;
				drawStartHighlight = false;
				drawSettingHighlight = false;

				return 1;
			}

			else{
				drawStartHighlight = false;
				drawEndHighlight = false;
				drawSettingHighlight = false;

				return 0;
			}
		}
		else{
			drawStartHighlight = false;
			drawEndHighlight = false;
			drawSettingHighlight = false;

			return 0;
		}
	}

	int checkClick(float x, float y){

		//Check the x bounds first cause all buttons are the same width
		if ((x > 0.42) && (x < 0.57)){

			//Spot for first button
			if ((y > 0.35) && (y < 0.47)){
				return 1;
			}

			//Spot for second button
			else if ((y > 0.53) && (y < 0.64)){
				return 3;
			}

			else if ((y > 0.71) && (y < 0.81))
			{
				return 2;
			}

			else if ((y > 0.19) && (y < 0.22))
			{
				return 4;
			}

			return 0;
		}

		return 0;
	}

private:

	UI_Panel * menu;
	UI_Panel * game_name;
	UI_Panel * start_button;
	UI_Panel * exit_button;
	UI_Panel * setting_button;
	UI_Panel * selected_start_button;
	UI_Panel * selected_end_button;
	UI_Panel * selected_setting_button;
	UI_Panel * address_bar;

	bool drawStartHighlight = false;
	bool drawEndHighlight = false;
	bool drawSettingHighlight = false;
};

class GameMenu
{
public:
	GameMenu(){


		quit = new UI_Panel(-0.15f, 0.15f, -0.05f, 0.05f);
		quit->setColor(vec3(1.0, 0.0, 0.0));
		quit->setShader(sdrCtl.getShader("basic_2D"));
		quit->loadColorTex("img/UI_elements/button_stainlessSteel_QuitOFF.png", "PNG");
		quit->setTex(true);
		quit->setModelM(glm::scale(vec3(1.0f, 1.0f, 1.0))*glm::translate(vec3(-0.3f, 0.0, -1.0f)));

		resume = new UI_Panel(-0.15f, 0.15f, -0.05f, 0.05f);
		resume->setColor(vec3(1.0, 0.0, 0.0));
		resume->setShader(sdrCtl.getShader("basic_2D"));
		resume->loadColorTex("img/UI_elements/button_stainlessSteel_ResumeOFF.png", "PNG");
		resume->setTex(true);
		resume->setModelM(glm::scale(vec3(1.0f, 1.0f, 1.0))*glm::translate(vec3(0.3f, 0.0, -1.0f)));

		selected_resume = new UI_Panel(-0.15f, 0.15f, -0.05f, 0.05f);
		selected_resume->setColor(vec3(1.0, 0.0, 0.0));
		selected_resume->setShader(sdrCtl.getShader("basic_2D"));
		selected_resume->loadColorTex("img/UI_elements/button_stainlessSteel_ResumeON.png", "PNG");
		selected_resume->setTex(true);
		selected_resume->setModelM(glm::scale(vec3(1.0f, 1.0f, 1.0))*glm::translate(vec3(0.3f, 0.0, -1.0f)));

		selected_quit = new UI_Panel(-0.15f, 0.15f, -0.05f, 0.05f);
		selected_quit->setColor(vec3(1.0, 0.0, 0.0));
		selected_quit->setShader(sdrCtl.getShader("basic_2D"));
		selected_quit->loadColorTex("img/UI_elements/button_stainlessSteel_QuitON.png", "PNG");
		selected_quit->setTex(true);
		selected_quit->setModelM(glm::scale(vec3(1.0f, 1.0f, 1.0))*glm::translate(vec3(-0.3f, 0.0, -1.0f)));


		frame = new UI_Panel(-0.5f, 0.5f, -0.25, 0.25);
		frame->setColor(vec3(1.0, 1.0, 1.0));
		frame->setShader(sdrCtl.getShader("basic_2D"));
		frame->loadColorTex("img/UI_elements/glassPanel.png", "PNG");
		frame->setTex(true);
		frame->setModelM(glm::scale(vec3(1.0f, 1.0f, 1.0))*glm::translate(vec3(0.0f, 0.0, -1.0f)));

	}
	~GameMenu(){

		resume->			~UI_Panel();
		quit->				~UI_Panel();
		selected_resume->	~UI_Panel();
		selected_quit->		~UI_Panel();
		frame->				~UI_Panel();
	}

	int draw(){
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		frame->draw();

		glDisable(GL_BLEND);

		glDisable(GL_DEPTH_TEST);

		//frame->draw();
		resume->draw();
		quit->draw();

		if (drawStartHighlight){
			selected_resume->draw();
		}

		if (drawEndHighlight){
			selected_quit->draw();
		}

		glEnable(GL_DEPTH_TEST);
		return 0;
	}

	int checkHighlight(float x, float y){

		//Check the x bounds first cause all buttons are the same width
		if ((y > 0.46) && (y < 0.53)){

			//Spot for first button
			if ((x > 0.58) && (x < 0.73)){
				drawStartHighlight = true;
				drawEndHighlight = false;

				return 1;
				//cout << "Start Monica!" << endl;
			}

			//Spot for second button
			else if ((x > 0.26) && (x < 0.41)){
				drawEndHighlight = true;
				drawStartHighlight = false;

				return 1;
				//cout << "End Monica!" << endl;
			}
			else{
				drawStartHighlight = false;
				drawEndHighlight = false;

				return 0;
			}
		}
		else{
			drawStartHighlight = false;
			drawEndHighlight = false;

			return 0;
		}
	}

	int checkClick(float x, float y){

		//Check the x bounds first cause all buttons are the same width
		if ((y > 0.46) && (y < 0.53)){

			//Spot for first button
			if ((x > 0.58) && (x < 0.73)){
				return 1;
			}

			//Spot for second button
			else if ((x > 0.26) && (x < 0.41)){
				return 2;
			}

			return 0;
		}

		return 0;
	}

private:

	float highlightTrans = 0;

	UI_Panel * resume;
	UI_Panel * quit;
	UI_Panel * selected_resume;
	UI_Panel * selected_quit;
	UI_Panel * frame;

	bool drawStartHighlight;
	bool drawEndHighlight;
};

class DeathScreen
{
public:
	DeathScreen(){
		respawn = new UI_Panel(-0.15f, 0.15f, -0.05f, 0.05f);
		respawn->setColor(vec3(1.0, 0.0, 0.0));
		respawn->setShader(sdrCtl.getShader("basic_2D"));
		respawn->loadColorTex("img/UI_elements/button_stainlessSteel_ResumeOFF.png", "PNG");
		respawn->setTex(true);
		respawn->setModelM(glm::scale(vec3(1.0f, 1.0f, 1.0))*glm::translate(vec3(0.0f, -0.1f, -1.0f)));

		selected_respawn = new UI_Panel(-0.15f, 0.15f, -0.05f, 0.05f);
		selected_respawn->setColor(vec3(1.0, 0.0, 0.0));
		selected_respawn->setShader(sdrCtl.getShader("basic_2D"));
		selected_respawn->loadColorTex("img/UI_elements/button_stainlessSteel_ResumeON.png", "PNG");
		selected_respawn->setTex(true);
		selected_respawn->setModelM(glm::scale(vec3(1.0f, 1.0f, 1.0))*glm::translate(vec3(0.0f, -0.1, -1.0f)));

		frame = new UI_Panel(-0.5f, 0.5f, -0.5, 0.5);
		frame->setColor(vec3(1.0, 0.0, 0.0));
		frame->setShader(sdrCtl.getShader("basic_2D"));
		frame->loadColorTex("img/UI_elements/glassPanel.png", "PNG");
		frame->setTex(true);
		frame->setModelM(glm::scale(vec3(1.0f, 1.0f, 1.0))*glm::translate(vec3(0.0f, 0.0, -1.0f)));

	}
	~DeathScreen(){
		respawn->           ~UI_Panel();
		selected_respawn->  ~UI_Panel();
		frame->             ~UI_Panel();
	}

	int draw(){


		unsigned char one[] = "1";
		unsigned char two[] = "2";
		unsigned char three[] = "3";
		unsigned char four[] = "4";
		unsigned char five[] = "5";
		unsigned char six[] = "6";
		unsigned char seven[] = "7";
		unsigned char eight[] = "8";
		unsigned char nine[] = "9";
		unsigned char ready[] = "NOW";
		unsigned char s[] = "RESPAWN READY IN:";

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		frame->draw();

		glDisable(GL_BLEND);

		glDisable(GL_DEPTH_TEST);

		respawn->draw();

		if (drawRespawnHighlight){
			selected_respawn->draw();
		}

		i = (clock() - since_death) / (float)CLOCKS_PER_SEC;

		RenderString((Window::width / 2) - 124, 5* Window::height / 8, GLUT_BITMAP_TIMES_ROMAN_24, s, vec3(1.0f, 1.0f, 1.0f));
			switch (i){
			case 9:
				RenderString((Window::width / 2) - 24, 9 * Window::height / 16, GLUT_BITMAP_TIMES_ROMAN_24, ready, vec3(1.0f, 1.0f, 1.0f));
				break;
			case 8:
				RenderString(Window::width / 2, 9*Window::height / 16, GLUT_BITMAP_TIMES_ROMAN_24, one, vec3(1.0f, 1.0f, 1.0f));
				break;
			case 7:
				RenderString(Window::width / 2, 9 * Window::height / 16, GLUT_BITMAP_TIMES_ROMAN_24, two, vec3(1.0f, 1.0f, 1.0f));
				break;
			case 6:
				RenderString(Window::width / 2, 9 * Window::height / 16, GLUT_BITMAP_TIMES_ROMAN_24, three, vec3(1.0f, 1.0f, 1.0f));
				break;
			case 5:
				RenderString(Window::width / 2, 9 * Window::height / 16, GLUT_BITMAP_TIMES_ROMAN_24, four, vec3(1.0f, 1.0f, 1.0f));
				break;
			case 4:
				RenderString(Window::width / 2, 9 * Window::height / 16, GLUT_BITMAP_TIMES_ROMAN_24, five, vec3(1.0f, 1.0f, 1.0f));
				break;
			case 3:
				RenderString(Window::width / 2, 9 * Window::height / 16, GLUT_BITMAP_TIMES_ROMAN_24, six, vec3(1.0f, 1.0f, 1.0f));
				break;
			case 2:
				RenderString(Window::width / 2, 9 * Window::height / 16, GLUT_BITMAP_TIMES_ROMAN_24, seven, vec3(1.0f, 1.0f, 1.0f));
				break;
			case 1:
				RenderString(Window::width / 2, 9 * Window::height / 16, GLUT_BITMAP_TIMES_ROMAN_24, eight, vec3(1.0f, 1.0f, 1.0f));
				break;
			case 0:
				RenderString(Window::width / 2, 9 * Window::height / 16, GLUT_BITMAP_TIMES_ROMAN_24, nine, vec3(1.0f, 1.0f, 1.0f));
				break;
			default:
				RenderString(Window::width / 2 - 24, 9 * Window::height / 16, GLUT_BITMAP_TIMES_ROMAN_24, ready, vec3(1.0f, 1.0f, 1.0f));
				break;
			}
		
		glEnable(GL_DEPTH_TEST);
		return 0;
	}

	int checkHighlight(float x, float y){

		//Check the x bounds first cause all buttons are the same width
		if ((x > 0.42) && (x < 0.57)){

			if ((y > 0.53) && (y < 0.64) && i > 8){
				drawRespawnHighlight = true;
				return 1;
			}

			else{
				drawRespawnHighlight = false;
				return 0;
			}
		}

		else{
			drawRespawnHighlight = false;
			return 0;
		}
	}

	int checkClick(float x, float y){

		//Check the x bounds first cause all buttons are the same width
		if ((x > 0.42) && (x < 0.57)){

			if ((y > 0.53) && (y < 0.64) && i > 8){
				return 1;
			}
			else{
				return 0;
			}
		}

		else{
			return 0;
		}
	
	}

	void setDeathClock(float time){ since_death = time;}

private:
	UI_Panel * frame;
	UI_Panel * respawn;
	UI_Panel * selected_respawn;

	int i;
	float since_death;
	bool drawRespawnHighlight = false;
	
};

class Logo
{
public:
	Logo()
	{
		logo = new UI_Panel(-0.1f, 0.1f, -0.08f, 0.08f);
		logo->setColor(vec3(1.0, 0.0, 0.0));
		logo->setShader(sdrCtl.getShader("basic_2D"));
		logo->loadColorTex("img/UI_elements/logoB", "PNG");
		logo->setTex(true);
		logo->setModelM(glm::scale(vec3(1.0, 1.0, 1.0))*glm::translate(vec3(0.0f, -0.45, -1.0f)));
	}
	~Logo()
	{
		logo-> ~UI_Panel();
	}
	int draw()
	{
		glDisable(GL_DEPTH_TEST);

		logo->draw();

		glEnable(GL_DEPTH_TEST);

		return 0;
	}
private:
	UI_Panel * logo;
};

class Settings
{
public:
	Settings()
	{
		settings = new UI_Panel(-0.5f, 0.5f, -0.5f, 0.5f);
		settings->setColor(vec3(1.0, 0.0, 0.0));
		settings->setShader(sdrCtl.getShader("basic_2D"));
		settings->loadColorTex("img/UI_elements/logoB", "PNG");
		settings->setTex(true);
		settings->setModelM(glm::scale(vec3(1.0, 1.0, 1.0))*glm::translate(vec3(0.0f, 0.0f, -1.0f)));

		address_bar = new UI_Panel(-1, 1, -1, 1);
		address_bar->setColor(vec3(1.0, 0.0, 0.0));
		address_bar->setShader(sdrCtl.getShader("basic_2D"));
		address_bar->loadColorTex("img/UI_elements/IP_adressBAR", "PNG");
		address_bar->setTex(true);
		address_bar->setModelM(glm::scale(vec3(0.35, 0.05, 1.0))*glm::translate(vec3(0.0f, 0.0f, -1.0f)));
	}

	~Settings()
	{
		settings->    ~UI_Panel();
		address_bar-> ~UI_Panel();
	}

	int draw()
	{
		glDisable(GL_DEPTH_TEST);

		settings->draw();
		//address_bar->draw();

		glEnable(GL_DEPTH_TEST);

		return 0;
	}
private:
	UI_Panel * settings;
	UI_Panel * address_bar;

};

class End_Screen
{
public:
	End_Screen()
	{
		victory = new UI_Panel(-0.3f, 0.3f, -0.1, 0.1);
		victory->setColor(vec3(1.0, 0.0, 0.0));
		victory->setShader(sdrCtl.getShader("basic_2D"));
		victory->loadColorTex("img/UI_elements/victory.png", "PNG");
		victory->setTex(true);
		victory->setModelM(glm::scale(vec3(1.0, 1.0, 1.0))*glm::translate(vec3(0.0f, 0.0, -1.0f)));

		defeat = new UI_Panel(-0.3f, 0.3f, -0.1, 0.1);
		defeat->setColor(vec3(1.0, 0.0, 0.0));
		defeat->setShader(sdrCtl.getShader("basic_2D"));
		defeat->loadColorTex("img/UI_elements/defeat.png", "PNG");
		defeat->setTex(true);
		defeat->setModelM(glm::scale(vec3(1.0, 1.0, 1.0))*glm::translate(vec3(0.0f, 0.0, -1.0f)));

		button = new UI_Panel(-0.15f, 0.15f, -0.05f, 0.05f);
		button->setColor(vec3(1.0, 0.0, 0.0));
		button->setShader(sdrCtl.getShader("basic_2D"));
		button->loadColorTex("img/UI_elements/button_stainlessSteel_ExitOFF_1.png", "PNG");
		button->setTex(true);
		button->setModelM(glm::scale(vec3(1.0, 1.0, 1.0))*glm::translate(vec3(0.0f, -0.3, -1.0f)));

		selected_button = new UI_Panel(-0.15f, 0.15f, -0.05f, 0.05f);
		selected_button->setColor(vec3(1.0, 0.0, 0.0));
		selected_button->setShader(sdrCtl.getShader("basic_2D"));
		selected_button->loadColorTex("img/UI_elements/button_stainlessSteel_SettingsON", "PNG");
		selected_button->setTex(true);
		selected_button->setModelM(glm::scale(vec3(1.0, 1.0, 1.0))*glm::translate(vec3(0.0f, -0.3, -1.0f)));

	}

	~End_Screen()
	{
		victory->          ~UI_Panel();
		defeat->           ~UI_Panel();
		button->           ~UI_Panel();
		selected_button->  ~UI_Panel();
	}

	int draw(int x)
	{

		glDisable(GL_DEPTH_TEST);

		if (x)
		{
			victory->draw();
		}
		else
		{
			defeat->draw();
		}
	
		button->draw();

		if (drawButtonHighlight){
			selected_button->draw();
		}

		glEnable(GL_DEPTH_TEST);

		return 0;
	}

	int checkHighlight(float x, float y){

		//Check the x bounds first cause all buttons are the same width
		if ((x > 0.42) && (x < 0.57)){

			if ((y > 0.71) && (y < 0.81)){
				drawButtonHighlight = true;
				return 1;
			}

			else{
				drawButtonHighlight = false;
				return 0;
			}
		}

		else{
			drawButtonHighlight = false;
			return 0;
		}
	}

	int checkClick(float x, float y){

		//Check the x bounds first cause all buttons are the same width
		if ((x > 0.42) && (x < 0.57)){

			if ((y > 0.71) && (y < 0.81)){
				return 1;
			}
			else{
				return 0;
			}
		}

		else{
			return 0;
		}

	}

private:
	UI_Panel * victory;
	UI_Panel * defeat;
	UI_Panel * button;
	UI_Panel * selected_button;

	bool drawButtonHighlight = false;
};