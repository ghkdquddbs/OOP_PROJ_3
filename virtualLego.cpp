////////////////////////////////////////////////////////////////////////////////
//
// File: virtualLego.cpp
//
// Original Author: 박창현 Chang-hyeon Park, 
// Modified by Bong-Soo Sohn and Dong-Jun Kim
// 
// Originally programmed for Virtual LEGO. 
// Modified later to program for Virtual Billiard.
//        
////////////////////////////////////////////////////////////////////////////////
#include "d3dUtility.h"
#include <vector>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <cassert>

IDirect3DDevice9* Device = NULL;

// window size
const int Width = 1024;
const int Height = 768;

// There are four balls
// initialize the position (coordinate) of each ball (ball0 ~ ball3)
const float spherePos[7][2] = { {-1.2f,0} , {-0.2f,-2.0f}, {-0.2f,-1.0f}, {-0.2f,0}, {-0.2f,+1.0f}, {-0.2f,+2.0f} , {-3.3f,0} };
// initialize the color of each ball (ball0 ~ ball3)
const D3DXCOLOR sphereColor[7] = { d3d::RED, d3d::YELLOW, d3d::YELLOW, d3d::YELLOW, d3d::YELLOW, d3d::YELLOW, d3d::WHITE };

// -----------------------------------------------------------------------------
// Transform matrices
// -----------------------------------------------------------------------------
D3DXMATRIX g_mWorld;
D3DXMATRIX g_mView;
D3DXMATRIX g_mProj;

#define M_RADIUS 0.21   // ball radius
#define PI 3.14159265
#define M_HEIGHT 0.01
#define DECREASE_RATE 0.9982
//added preprocessors
#define BLOCK_WIDTH 0.8f
#define BLOCK_HEIGHT 0.4f
#define PADDLE_WIDTH 2.0f
#define PADDLE_HEIGHT 0.3f
#define BALL_SPEED 3.0f
#define PADDLE_SPEED 4.0f

// -----------------------------------------------------------------------------
// CSphere class definition
// -----------------------------------------------------------------------------

class CSphere {
private:
	float					center_x, center_y, center_z;
	float                   m_radius;
	float					m_velocity_x;
	float					m_velocity_z;

	bool active=true;

public:
	CSphere(void)
	{
		D3DXMatrixIdentity(&m_mLocal);
		ZeroMemory(&m_mtrl, sizeof(m_mtrl));
		m_radius = 0;
		m_velocity_x = 0;
		m_velocity_z = 0;
		m_pSphereMesh = NULL;
	}
	~CSphere(void) {}

public:
	bool create(IDirect3DDevice9* pDevice, D3DXCOLOR color = d3d::WHITE)
	{
		if (NULL == pDevice)
			return false;

		m_mtrl.Ambient = color;
		m_mtrl.Diffuse = color;
		m_mtrl.Specular = color;
		m_mtrl.Emissive = d3d::BLACK;
		m_mtrl.Power = 5.0f;

		if (FAILED(D3DXCreateSphere(pDevice, getRadius(), 50, 50, &m_pSphereMesh, NULL)))
			return false;
		return true;
	}

	void destroy(void)
	{
		if (m_pSphereMesh != NULL) {
			m_pSphereMesh->Release();
			m_pSphereMesh = NULL;
		}
	}

	/*void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
	{
		if (NULL == pDevice)
			return;
		pDevice->SetTransform(D3DTS_WORLD, &mWorld);
		pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
		pDevice->SetMaterial(&m_mtrl);
		m_pSphereMesh->DrawSubset(0);
	}*/
	void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
	{
		if (!active || NULL == pDevice || m_pSphereMesh == NULL) return; // Skip inactive or invalid balls
		pDevice->SetTransform(D3DTS_WORLD, &mWorld);
		pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
		pDevice->SetMaterial(&m_mtrl);
		m_pSphereMesh->DrawSubset(0);
	}

	bool hasIntersected(CSphere& ball)
	{
		// Insert your code here.
		// Get the centers of both spheres
		D3DXVECTOR3 thisCenter = this->getCenter();
		D3DXVECTOR3 ballCenter = ball.getCenter();

		// Calculate the distance between the centers
		float distance = sqrt(pow(thisCenter.x - ballCenter.x, 2) +
			pow(thisCenter.y - ballCenter.y, 2) +
			pow(thisCenter.z - ballCenter.z, 2));

		// Check if the distance is less than or equal to the sum of their radii
		return distance <= (this->getRadius() + ball.getRadius());
	}

	//추가
	bool isActive() const { return active; }
	bool isColorEqual(const D3DXCOLOR& color1, const D3DXCOLOR& color2, float tolerance = 0.01f)
	{
		return fabs(color1.r - color2.r) <= tolerance &&
			fabs(color1.g - color2.g) <= tolerance &&
			fabs(color1.b - color2.b) <= tolerance &&
			fabs(color1.a - color2.a) <= tolerance;
	}

	void hitBy(CSphere& ball)
	{
		if (!this->active || !ball.active) return; // Skip if either ball is inactive
		if (!this->hasIntersected(ball)) return;  // No collision, return early

		// Handle red ball hitting yellow ball logic
		else if (isColorEqual(this->m_mtrl.Diffuse, d3d::YELLOW) && isColorEqual(ball.m_mtrl.Diffuse, d3d::RED))
		{
			(this->active) = false; // Mark the yellow ball as removed
			ball.setPower(-ball.getVelocity_X(), -ball.getVelocity_Z()); // Bounce red ball back
			return;
		}

		else {
			// Handle other collision logic here (e.g., normal ball interactions)
			D3DXVECTOR3 thisCenter = this->getCenter();
			D3DXVECTOR3 ballCenter = ball.getCenter();

			D3DXVECTOR3 normal = thisCenter - ballCenter;
			D3DXVec3Normalize(&normal, &normal);

			D3DXVECTOR3 tangent(-normal.z, 0, normal.x);

			float thisNormalVel = D3DXVec3Dot(&normal, &D3DXVECTOR3(this->getVelocity_X(), 0, this->getVelocity_Z()));
			float ballNormalVel = D3DXVec3Dot(&normal, &D3DXVECTOR3(ball.getVelocity_X(), 0, ball.getVelocity_Z()));

			float thisTangentVel = D3DXVec3Dot(&tangent, &D3DXVECTOR3(this->getVelocity_X(), 0, this->getVelocity_Z()));
			float ballTangentVel = D3DXVec3Dot(&tangent, &D3DXVECTOR3(ball.getVelocity_X(), 0, ball.getVelocity_Z()));

			std::swap(thisNormalVel, ballNormalVel);

			D3DXVECTOR3 thisNewVel = thisNormalVel * normal + thisTangentVel * tangent;
			D3DXVECTOR3 ballNewVel = ballNormalVel * normal + ballTangentVel * tangent;

			this->setPower(thisNewVel.x, thisNewVel.z);
			ball.setPower(ballNewVel.x, ballNewVel.z);
		}
	}

	//void hitBy(CSphere& ball)
	//{
	//	// Insert your code here.
	//	if (!this->active || !ball.active) return; // Skip if either ball is inactive
	//	if (!this->hasIntersected(ball)) return; // No collision, return early

	//	// Get centers of spheres
	//	D3DXVECTOR3 thisCenter = this->getCenter();
	//	D3DXVECTOR3 ballCenter = ball.getCenter();

	//	// Calculate normal and tangent directions
	//	D3DXVECTOR3 normal = thisCenter - ballCenter;
	//	D3DXVec3Normalize(&normal, &normal); // Normalize to unit vector

	//	D3DXVECTOR3 tangent(-normal.z, 0, normal.x); // Tangent is perpendicular to normal

	//	// Calculate velocities along the normal and tangent directions
	//	float thisNormalVel = D3DXVec3Dot(&normal, &D3DXVECTOR3(this->getVelocity_X(), 0, this->getVelocity_Z()));
	//	float ballNormalVel = D3DXVec3Dot(&normal, &D3DXVECTOR3(ball.getVelocity_X(), 0, ball.getVelocity_Z()));

	//	float thisTangentVel = D3DXVec3Dot(&tangent, &D3DXVECTOR3(this->getVelocity_X(), 0, this->getVelocity_Z()));
	//	float ballTangentVel = D3DXVec3Dot(&tangent, &D3DXVECTOR3(ball.getVelocity_X(), 0, ball.getVelocity_Z()));

	//	// Swap normal velocities (elastic collision)
	//	std::swap(thisNormalVel, ballNormalVel);

	//	// Compute the new velocities for both spheres
	//	D3DXVECTOR3 thisNewVel = thisNormalVel * normal + thisTangentVel * tangent;
	//	D3DXVECTOR3 ballNewVel = ballNormalVel * normal + ballTangentVel * tangent;

	//	// Set the new velocities
	//	this->setPower(thisNewVel.x, thisNewVel.z);
	//	ball.setPower(ballNewVel.x, ballNewVel.z);
	//}


	void ballUpdate(float timeDiff)
	{
		const float TIME_SCALE = 3.3;
		D3DXVECTOR3 cord = this->getCenter();
		double vx = abs(this->getVelocity_X());
		double vz = abs(this->getVelocity_Z());

		if (vx > 0.01 || vz > 0.01)
		{
			float tX = cord.x + TIME_SCALE * timeDiff * m_velocity_x;
			float tZ = cord.z + TIME_SCALE * timeDiff * m_velocity_z;

			//correction of position of ball
			// Please uncomment this part because this correction of ball position is necessary when a ball collides with a wall
			if (tX >= (4.5 - M_RADIUS))
				tX = 4.5 - M_RADIUS;
			else if (tX <= (-4.5 + M_RADIUS))
				tX = -4.5 + M_RADIUS;
			else if (tZ <= (-3 + M_RADIUS))
				tZ = -3 + M_RADIUS;
			else if (tZ >= (3 - M_RADIUS))
				tZ = 3 - M_RADIUS;

			this->setCenter(tX, cord.y, tZ);
		}
		else { this->setPower(0, 0); }
		//this->setPower(this->getVelocity_X() * DECREASE_RATE, this->getVelocity_Z() * DECREASE_RATE);
		double rate = 1 - (1 - DECREASE_RATE) * timeDiff * 400;
		if (rate < 0)
			rate = 0;
		this->setPower(getVelocity_X() * rate, getVelocity_Z() * rate);
	}

	double getVelocity_X() { return this->m_velocity_x; }
	double getVelocity_Z() { return this->m_velocity_z; }

	void setPower(double vx, double vz)
	{
		this->m_velocity_x = vx;
		this->m_velocity_z = vz;
	}

	void setCenter(float x, float y, float z)
	{
		D3DXMATRIX m;
		center_x = x;	center_y = y;	center_z = z;
		D3DXMatrixTranslation(&m, x, y, z);
		setLocalTransform(m);
	}

	float getRadius(void)  const { return (float)(M_RADIUS); }
	const D3DXMATRIX& getLocalTransform(void) const { return m_mLocal; }
	void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }
	D3DXVECTOR3 getCenter(void) const
	{
		D3DXVECTOR3 org(center_x, center_y, center_z);
		return org;
	}

private:
	D3DXMATRIX              m_mLocal;
	D3DMATERIAL9            m_mtrl;
	ID3DXMesh* m_pSphereMesh;

};



// -----------------------------------------------------------------------------
// CWall class definition
// -----------------------------------------------------------------------------

class CWall {

private:

	float					m_x;
	float					m_z;
	float                   m_width;
	float                   m_depth;
	float					m_height;

public:
	CWall(void)
	{
		D3DXMatrixIdentity(&m_mLocal);
		ZeroMemory(&m_mtrl, sizeof(m_mtrl));
		m_width = 0;
		m_depth = 0;
		m_pBoundMesh = NULL;
	}
	~CWall(void) {}
public:
	bool create(IDirect3DDevice9* pDevice, float ix, float iz, float iwidth, float iheight, float idepth, D3DXCOLOR color = d3d::WHITE)
	{
		if (NULL == pDevice)
			return false;

		m_mtrl.Ambient = color;
		m_mtrl.Diffuse = color;
		m_mtrl.Specular = color;
		m_mtrl.Emissive = d3d::BLACK;
		m_mtrl.Power = 5.0f;

		m_width = iwidth;
		m_depth = idepth;

		if (FAILED(D3DXCreateBox(pDevice, iwidth, iheight, idepth, &m_pBoundMesh, NULL)))
			return false;
		return true;
	}
	void destroy(void)
	{
		if (m_pBoundMesh != NULL) {
			m_pBoundMesh->Release();
			m_pBoundMesh = NULL;
		}
	}
	void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
	{
		if (NULL == pDevice)
			return;
		pDevice->SetTransform(D3DTS_WORLD, &mWorld);
		pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
		pDevice->SetMaterial(&m_mtrl);
		m_pBoundMesh->DrawSubset(0);
	}

	bool hasIntersected(CSphere& ball)
	{
		// Insert your code here.
		D3DXVECTOR3 ballCenter = ball.getCenter();
		float ballRadius = ball.getRadius();

		// Check for collision with this wall (based on its position and size)
		if (ballCenter.x - ballRadius <= -4.5f || ballCenter.x + ballRadius >= 4.5f ||
			ballCenter.z - ballRadius <= -3.0f || ballCenter.z + ballRadius >= 3.0f) {
			return true; // Collision detected
		}
		return false; // No collision
	}


	void CWall::hitBy(CSphere& ball)
	{
		if (!this->hasIntersected(ball)) return; // No collision, return early

		D3DXVECTOR3 ballCenter = ball.getCenter();
		float ballRadius = ball.getRadius();

		// Check collisions with walls and adjust velocity and position
		if (ballCenter.x - ballRadius <= -4.5f) { // Left wall
			ball.setPower(-abs(ball.getVelocity_X()), ball.getVelocity_Z()); // Reflect X velocity
			ball.setCenter(-4.5f + ballRadius, ballCenter.y, ballCenter.z); // Reposition outside wall
		}
		if (ballCenter.x + ballRadius >= 4.5f) { // Right wall
			ball.setPower(abs(ball.getVelocity_X()), ball.getVelocity_Z()); // Reflect X velocity
			ball.setCenter(4.5f - ballRadius, ballCenter.y, ballCenter.z); // Reposition outside wall
		}
		if (ballCenter.z - ballRadius <= -3.0f) { // Bottom wall
			ball.setPower(ball.getVelocity_X(), abs(ball.getVelocity_Z())); // Reflect Z velocity
			ball.setCenter(ballCenter.x, ballCenter.y, -3.0f + ballRadius); // Reposition outside wall
		}
		if (ballCenter.z + ballRadius >= 3.0f) { // Top wall
			ball.setPower(ball.getVelocity_X(), -abs(ball.getVelocity_Z())); // Reflect Z velocity
			ball.setCenter(ballCenter.x, ballCenter.y, 3.0f - ballRadius); // Reposition outside wall
		}
	}


	void setPosition(float x, float y, float z)
	{
		D3DXMATRIX m;
		this->m_x = x;
		this->m_z = z;

		D3DXMatrixTranslation(&m, x, y, z);
		setLocalTransform(m);
	}

	float getHeight(void) const { return M_HEIGHT; }



private:
	void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }

	D3DXMATRIX              m_mLocal;
	D3DMATERIAL9            m_mtrl;
	ID3DXMesh* m_pBoundMesh;
};

// -----------------------------------------------------------------------------
// CLight class definition
// -----------------------------------------------------------------------------

class CLight {
public:
	CLight(void)
	{
		static DWORD i = 0;
		m_index = i++;
		D3DXMatrixIdentity(&m_mLocal);
		::ZeroMemory(&m_lit, sizeof(m_lit));
		m_pMesh = NULL;
		m_bound._center = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
		m_bound._radius = 0.0f;
	}
	~CLight(void) {}
public:
	bool create(IDirect3DDevice9* pDevice, const D3DLIGHT9& lit, float radius = 0.1f)
	{
		if (NULL == pDevice)
			return false;
		if (FAILED(D3DXCreateSphere(pDevice, radius, 10, 10, &m_pMesh, NULL)))
			return false;

		m_bound._center = lit.Position;
		m_bound._radius = radius;

		m_lit.Type = lit.Type;
		m_lit.Diffuse = lit.Diffuse;
		m_lit.Specular = lit.Specular;
		m_lit.Ambient = lit.Ambient;
		m_lit.Position = lit.Position;
		m_lit.Direction = lit.Direction;
		m_lit.Range = lit.Range;
		m_lit.Falloff = lit.Falloff;
		m_lit.Attenuation0 = lit.Attenuation0;
		m_lit.Attenuation1 = lit.Attenuation1;
		m_lit.Attenuation2 = lit.Attenuation2;
		m_lit.Theta = lit.Theta;
		m_lit.Phi = lit.Phi;
		return true;
	}
	void destroy(void)
	{
		if (m_pMesh != NULL) {
			m_pMesh->Release();
			m_pMesh = NULL;
		}
	}
	bool setLight(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
	{
		if (NULL == pDevice)
			return false;

		D3DXVECTOR3 pos(m_bound._center);
		D3DXVec3TransformCoord(&pos, &pos, &m_mLocal);
		D3DXVec3TransformCoord(&pos, &pos, &mWorld);
		m_lit.Position = pos;

		pDevice->SetLight(m_index, &m_lit);
		pDevice->LightEnable(m_index, TRUE);
		return true;
	}

	void draw(IDirect3DDevice9* pDevice)
	{
		if (NULL == pDevice)
			return;
		D3DXMATRIX m;
		D3DXMatrixTranslation(&m, m_lit.Position.x, m_lit.Position.y, m_lit.Position.z);
		pDevice->SetTransform(D3DTS_WORLD, &m);
		pDevice->SetMaterial(&d3d::WHITE_MTRL);
		m_pMesh->DrawSubset(0);
	}

	D3DXVECTOR3 getPosition(void) const { return D3DXVECTOR3(m_lit.Position); }

private:
	DWORD               m_index;
	D3DXMATRIX          m_mLocal;
	D3DLIGHT9           m_lit;
	ID3DXMesh* m_pMesh;
	d3d::BoundingSphere m_bound;
};


// -----------------------------------------------------------------------------
// Global variables
// -----------------------------------------------------------------------------
CWall	g_legoPlane;
CWall	g_legowall[4];
CSphere	g_sphere[7];
CSphere	g_target_blueball;
CLight	g_light;

double g_camera_pos[3] = { 0.0, 5.0, -8.0 };

// -----------------------------------------------------------------------------
// Functions
// -----------------------------------------------------------------------------


void destroyAllLegoBlock(void)
{
}

// initialization
bool Setup()
{
	int i;

	D3DXMatrixIdentity(&g_mWorld);
	D3DXMatrixIdentity(&g_mView);
	D3DXMatrixIdentity(&g_mProj);

	// create plane and set the position
	if (false == g_legoPlane.create(Device, -1, -1, 9, 0.03f, 6, d3d::GREEN)) return false;
	g_legoPlane.setPosition(0.0f, -0.0006f / 5, 0.0f);

	// create walls and set the position. note that there are four walls
	if (false == g_legowall[0].create(Device, -1, -1, 9, 0.3f, 0.12f, d3d::DARKRED)) return false;
	g_legowall[0].setPosition(0.0f, 0.12f, 3.06f);
	if (false == g_legowall[1].create(Device, -1, -1, 9, 0.3f, 0.12f, d3d::DARKRED)) return false;
	g_legowall[1].setPosition(0.0f, 0.12f, -3.06f);
	if (false == g_legowall[2].create(Device, -1, -1, 0.12f, 0.3f, 6.24f, d3d::DARKRED)) return false;
	g_legowall[2].setPosition(4.56f, 0.12f, 0.0f);
	if (false == g_legowall[3].create(Device, -1, -1, 0.12f, 0.3f, 6.24f, d3d::DARKRED)) return false;
	g_legowall[3].setPosition(-4.56f, 0.12f, 0.0f);

	// create four balls and set the position
	for (i = 0; i < 7; i++) {
		if (false == g_sphere[i].create(Device, sphereColor[i])) return false;
		g_sphere[i].setCenter(spherePos[i][0], (float)M_RADIUS, spherePos[i][1]);
		g_sphere[i].setPower(0, 0);
	}

	// create blue ball for set direction
	if (false == g_target_blueball.create(Device, d3d::BLUE)) return false;
	g_target_blueball.setCenter(.0f, (float)M_RADIUS, .0f);

	// light setting 
	D3DLIGHT9 lit;
	::ZeroMemory(&lit, sizeof(lit));
	lit.Type = D3DLIGHT_POINT;
	lit.Diffuse = d3d::WHITE;
	lit.Specular = d3d::WHITE * 0.9f;
	lit.Ambient = d3d::WHITE * 0.9f;
	lit.Position = D3DXVECTOR3(0.0f, 3.0f, 0.0f);
	lit.Range = 100.0f;
	lit.Attenuation0 = 0.0f;
	lit.Attenuation1 = 0.9f;
	lit.Attenuation2 = 0.0f;
	if (false == g_light.create(Device, lit))
		return false;

	// Position and aim the camera.
	D3DXVECTOR3 pos(0.0f, 5.0f, -8.0f);
	D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 up(0.0f, 2.0f, 0.0f);
	D3DXMatrixLookAtLH(&g_mView, &pos, &target, &up);
	Device->SetTransform(D3DTS_VIEW, &g_mView);

	// Set the projection matrix.
	D3DXMatrixPerspectiveFovLH(&g_mProj, D3DX_PI / 4,
		(float)Width / (float)Height, 1.0f, 100.0f);
	Device->SetTransform(D3DTS_PROJECTION, &g_mProj);

	// Set render states.
	Device->SetRenderState(D3DRS_LIGHTING, TRUE);
	Device->SetRenderState(D3DRS_SPECULARENABLE, TRUE);
	Device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);

	g_light.setLight(Device, g_mWorld);
	return true;
}

void Cleanup(void)
{
	g_legoPlane.destroy();
	for (int i = 0; i < 4; i++) {
		g_legowall[i].destroy();
	}
	destroyAllLegoBlock();
	g_light.destroy();
}


// timeDelta represents the time between the current image frame and the last image frame.
// the distance of moving balls should be "velocity * timeDelta"
bool Display(float timeDelta)
{
	int i = 0;
	int j = 0;


	if (Device)
	{
		Device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00afafaf, 1.0f, 0);
		Device->BeginScene();

		//// update the position of each ball. during update, check whether each ball hit by walls.
		//for (i = 0; i < 7; i++) {
		//	g_sphere[i].ballUpdate(timeDelta);
		//	for (j = 0; j < 4; j++) { g_legowall[i].hitBy(g_sphere[j]); }
		//}

		//// check whether any two balls hit together and update the direction of balls
		//for (i = 0; i < 7; i++) {
		//	for (j = 0; j < 7; j++) {
		//		if (i >= j) { continue; }
		//		g_sphere[i].hitBy(g_sphere[j]);
		//	}
		//}

		for (i = 0; i < 7; i++) {
			if (!g_sphere[i].isActive()) continue; // Skip inactive balls
			g_sphere[i].ballUpdate(timeDelta);
			for (j = 0; j < 4; j++) {
				g_legowall[j].hitBy(g_sphere[i]);
			}
		}

		// Check collisions between balls
		for (i = 0; i < 7; i++) {
			if (!g_sphere[i].isActive()) continue; // Skip inactive balls
			for (j = 0; j < 7; j++) {
				if (i >= j || !g_sphere[j].isActive()) continue; // Skip inactive balls
				//g_sphere[i].hitBy(g_sphere[j]);
				if (g_sphere[i].hasIntersected(g_sphere[j])) {
					if (sphereColor[j] == d3d::YELLOW) { // Check if the ball is yellow
						g_sphere[i].hitBy(g_sphere[j]); // Ensure bounce happens
						//g_sphere[j].hitBy(g_sphere[i]); // Ensure both balls affect each other
						//g_sphere[j].active= false; // Mark the yellow ball as inactive
						g_sphere[j].destroy(); // Destroy the yellow ball
					}
					else {
						g_sphere[i].hitBy(g_sphere[j]); // Bounce for non-yellow balls
						//g_sphere[j].hitBy(g_sphere[i]);
					}
				}

			}
		}

		// draw plane, walls, and spheres
		g_legoPlane.draw(Device, g_mWorld);
		for (i = 0; i < 4; i++) {
			g_legowall[i].draw(Device, g_mWorld);
		}
		/*for (i = 0; i < 7; i++) {
			g_sphere[i].draw(Device, g_mWorld);
		}*/
		for (i = 0; i < 7; i++) {
			if (g_sphere[i].isActive()) { // Draw only active balls
				g_sphere[i].draw(Device, g_mWorld);
			}
		}
		g_target_blueball.draw(Device, g_mWorld);
		g_light.draw(Device);

		Device->EndScene();
		Device->Present(0, 0, 0, 0);
		Device->SetTexture(0, NULL);
	}
	return true;
}

LRESULT CALLBACK d3d::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static bool wire = false;
	static bool isReset = true;
	static int old_x = 0;
	static int old_y = 0;
	static enum { WORLD_MOVE, LIGHT_MOVE, BLOCK_MOVE } move = WORLD_MOVE;

	switch (msg) {
	case WM_DESTROY:
	{
		::PostQuitMessage(0);
		break;
	}
	case WM_KEYDOWN:
	{
		switch (wParam) {
		case VK_ESCAPE:
			::DestroyWindow(hwnd);
			break;
		case VK_RETURN:
			if (NULL != Device) {
				wire = !wire;
				Device->SetRenderState(D3DRS_FILLMODE,
					(wire ? D3DFILL_WIREFRAME : D3DFILL_SOLID));
			}
			break;
		case VK_SPACE:

			D3DXVECTOR3 targetpos = g_target_blueball.getCenter();
			D3DXVECTOR3	whitepos = g_sphere[6].getCenter();
			double theta = acos(sqrt(pow(targetpos.x - whitepos.x, 2)) / sqrt(pow(targetpos.x - whitepos.x, 2) +
				pow(targetpos.z - whitepos.z, 2)));		// 기본 1 사분면
			if (targetpos.z - whitepos.z <= 0 && targetpos.x - whitepos.x >= 0) { theta = -theta; }	//4 사분면
			if (targetpos.z - whitepos.z >= 0 && targetpos.x - whitepos.x <= 0) { theta = PI - theta; } //2 사분면
			if (targetpos.z - whitepos.z <= 0 && targetpos.x - whitepos.x <= 0) { theta = PI + theta; } // 3 사분면
			double distance = sqrt(pow(targetpos.x - whitepos.x, 2) + pow(targetpos.z - whitepos.z, 2));
			g_sphere[6].setPower(distance * cos(theta), distance * sin(theta));

			break;

		}
		break;
	}

	case WM_MOUSEMOVE:
	{
		int new_x = LOWORD(lParam);
		int new_y = HIWORD(lParam);
		float dx;
		float dy;

		if (LOWORD(wParam) & MK_LBUTTON) {

			if (isReset) {
				isReset = false;
			}
			else {
				D3DXVECTOR3 vDist;
				D3DXVECTOR3 vTrans;
				D3DXMATRIX mTrans;
				D3DXMATRIX mX;
				D3DXMATRIX mY;

				switch (move) {
				case WORLD_MOVE:
					dx = (old_x - new_x) * 0.01f;
					dy = (old_y - new_y) * 0.01f;
					D3DXMatrixRotationY(&mX, dx);
					D3DXMatrixRotationX(&mY, dy);
					g_mWorld = g_mWorld * mX * mY;

					break;
				}
			}

			old_x = new_x;
			old_y = new_y;

		}
		else {
			isReset = true;

			if (LOWORD(wParam) & MK_RBUTTON) {
				dx = (old_x - new_x);// * 0.01f;
				dy = (old_y - new_y);// * 0.01f;

				D3DXVECTOR3 coord3d = g_target_blueball.getCenter();
				g_target_blueball.setCenter(coord3d.x + dx * (-0.007f), coord3d.y, coord3d.z + dy * 0.007f);
			}
			old_x = new_x;
			old_y = new_y;

			move = WORLD_MOVE;
		}
		break;
	}
	}

	return ::DefWindowProc(hwnd, msg, wParam, lParam);
}


int WINAPI WinMain(_In_ HINSTANCE hinstance,
	_In_opt_ HINSTANCE prevInstance,
	_In_ PSTR cmdLine,
	_In_ int showCmd)
{
	srand(static_cast<unsigned int>(time(NULL)));

	if (!d3d::InitD3D(hinstance,
		Width, Height, true, D3DDEVTYPE_HAL, &Device))
	{
		::MessageBox(0, "InitD3D() - FAILED", 0, 0);
		return 0;
	}

	if (!Setup())
	{
		::MessageBox(0, "Setup() - FAILED", 0, 0);
		return 0;
	}

	d3d::EnterMsgLoop(Display);

	Cleanup();

	Device->Release();

	return 0;
}
