// Copyright (C) 2009-2015, Panagiotis Christopoulos Charitos.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include "anki/renderer/DebugDrawer.h"
#include "anki/renderer/Renderer.h"
#include "anki/resource/ShaderResource.h"
#include "anki/resource/TextureResource.h"
#include "anki/renderer/Renderer.h"
#include "anki/renderer/Pps.h"
#include "anki/renderer/Ms.h"
#include "anki/util/Logger.h"
#include "anki/physics/PhysicsWorld.h"
#include "anki/Collision.h"
#include "anki/Scene.h"

namespace anki {

//==============================================================================
// DebugDrawer                                                                 =
//==============================================================================

//==============================================================================
DebugDrawer::DebugDrawer()
{}

//==============================================================================
DebugDrawer::~DebugDrawer()
{}

//==============================================================================
Error DebugDrawer::create(Renderer* r)
{
	GrManager& gr = r->getGrManager();

	ANKI_CHECK(
		r->getResourceManager().loadResource("shaders/Dbg.vert.glsl", m_vert));
	ANKI_CHECK(
		r->getResourceManager().loadResource("shaders/Dbg.frag.glsl", m_frag));

	PipelineInitializer init;
	init.m_vertex.m_bindingCount = 1;
	init.m_vertex.m_bindings[0].m_stride = 2 * sizeof(Vec4);
	init.m_vertex.m_attributeCount = 2;
	init.m_vertex.m_attributes[0].m_format =
		PixelFormat(ComponentFormat::R32G32B32A32, TransformFormat::FLOAT);
	init.m_vertex.m_attributes[0].m_offset = 0;
	init.m_vertex.m_attributes[0].m_binding = 0;
	init.m_vertex.m_attributes[1].m_format =
		PixelFormat(ComponentFormat::R32G32B32A32, TransformFormat::FLOAT);
	init.m_vertex.m_attributes[1].m_offset = sizeof(Vec4);
	init.m_vertex.m_attributes[1].m_binding = 0;
	init.m_inputAssembler.m_topology = PrimitiveTopology::LINES;
	init.m_depthStencil.m_depthWriteEnabled = false;
	init.m_depthStencil.m_format = Ms::DEPTH_RT_PIXEL_FORMAT;
	init.m_color.m_attachmentCount = 1;
	init.m_color.m_attachments[0].m_format = Pps::RT_PIXEL_FORMAT;
	init.m_shaders[U(ShaderType::VERTEX)] = m_vert->getGrShader();
	init.m_shaders[U(ShaderType::FRAGMENT)] = m_frag->getGrShader();

	m_pplineLinesDepth = gr.newInstance<Pipeline>(init);

	init.m_depthStencil.m_depthCompareFunction = CompareOperation::ALWAYS;
	m_pplineLinesNoDepth = gr.newInstance<Pipeline>(init);

	m_vertBuff = gr.newInstance<Buffer>(sizeof(m_clientLineVerts),
		BufferUsageBit::VERTEX, BufferAccessBit::CLIENT_WRITE);

	ResourceGroupInitializer rcinit;
	rcinit.m_vertexBuffers[0].m_buffer = m_vertBuff;
	m_rcGroup = gr.newInstance<ResourceGroup>(rcinit);

	m_lineVertCount = 0;
	m_triVertCount = 0;
	m_mMat.setIdentity();
	m_vpMat.setIdentity();
	m_mvpMat.setIdentity();
	m_crntCol = Vec3(1.0, 0.0, 0.0);

	return ErrorCode::NONE;
}

//==============================================================================
void DebugDrawer::setModelMatrix(const Mat4& m)
{
	m_mMat = m;
	m_mvpMat = m_vpMat * m_mMat;
}

//==============================================================================
void DebugDrawer::setViewProjectionMatrix(const Mat4& m)
{
	m_vpMat = m;
	m_mvpMat = m_vpMat * m_mMat;
}

//==============================================================================
void DebugDrawer::begin(PrimitiveTopology topology)
{
	ANKI_ASSERT(topology == PrimitiveTopology::LINES
		|| topology == PrimitiveTopology::TRIANGLES);
	m_primitive = topology;
}

//==============================================================================
void DebugDrawer::end()
{
	if(m_primitive == PrimitiveTopology::LINES)
	{
		if(m_lineVertCount % 2 != 0)
		{
			pushBackVertex(Vec3(0.0));
			ANKI_LOGW("Forgot to close the line loop");
		}
	}
	else
	{
		if(m_triVertCount % 3 != 0)
		{
			pushBackVertex(Vec3(0.0));
			pushBackVertex(Vec3(0.0));
			ANKI_LOGW("Forgot to close the line loop");
		}
	}
}

//==============================================================================
Error DebugDrawer::flush()
{
	Error err = flushInternal(PrimitiveTopology::LINES);

	if(!err)
	{
		err = flushInternal(PrimitiveTopology::TRIANGLES);
	}

	return err;
}

//==============================================================================
Error DebugDrawer::flushInternal(PrimitiveTopology topology)
{
	if((m_primitive == PrimitiveTopology::LINES && m_lineVertCount == 0)
		|| (m_primitive == PrimitiveTopology::TRIANGLES && m_triVertCount == 0))
	{
		// Early exit
		return ErrorCode::NONE;
	}

	U clientVerts;
	void* vertBuff;
	if(m_primitive == PrimitiveTopology::LINES)
	{
		clientVerts = m_lineVertCount;
		vertBuff = &m_clientLineVerts[0];
		m_lineVertCount = 0;
	}
	else
	{
		clientVerts = m_triVertCount;
		vertBuff = &m_clientTriVerts[0];
		m_triVertCount = 0;
	}

	U size = sizeof(Vertex) * clientVerts;

	void* data = nullptr;
	m_cmdb->writeBuffer(m_vertBuff, 0, size, data);
	memcpy(data, vertBuff, size);

	if(m_depthTestEnabled)
	{
		m_cmdb->bindPipeline(m_pplineLinesDepth);
	}
	else
	{
		m_cmdb->bindPipeline(m_pplineLinesNoDepth);
	}

	m_cmdb->bindResourceGroup(m_rcGroup, 0, nullptr);
	m_cmdb->drawArrays(clientVerts);

	return ErrorCode::NONE;
}

//==============================================================================
void DebugDrawer::pushBackVertex(const Vec3& pos)
{
	U32* vertCount;
	Vertex* vertBuff;
	if(m_primitive == PrimitiveTopology::LINES)
	{
		vertCount = &m_lineVertCount;
		vertBuff = &m_clientLineVerts[0];
	}
	else
	{
		vertCount = &m_triVertCount;
		vertBuff = &m_clientTriVerts[0];
	}

	vertBuff[*vertCount].m_position = m_mvpMat * Vec4(pos, 1.0);
	vertBuff[*vertCount].m_color = Vec4(m_crntCol, 1.0);

	++(*vertCount);

	if(*vertCount == MAX_POINTS_PER_DRAW)
	{
		Error err = flush();
		// TODO Don't ignore
		(void)err;
	}
}

//==============================================================================
void DebugDrawer::drawLine(const Vec3& from, const Vec3& to, const Vec4& color)
{
	setColor(color);
	begin(PrimitiveTopology::LINES);
		pushBackVertex(from);
		pushBackVertex(to);
	end();
}

//==============================================================================
void DebugDrawer::drawGrid()
{
	Vec4 col0(0.5, 0.5, 0.5, 1.0);
	Vec4 col1(0.0, 0.0, 1.0, 1.0);
	Vec4 col2(1.0, 0.0, 0.0, 1.0);

	const F32 SPACE = 1.0; // space between lines
	const I NUM = 57;  // lines number. must be odd

	const F32 GRID_HALF_SIZE = ((NUM - 1) * SPACE / 2);

	setColor(col0);

	begin(PrimitiveTopology::LINES);

	for(I x = - NUM / 2 * SPACE; x < NUM / 2 * SPACE; x += SPACE)
	{
		setColor(col0);

		// if the middle line then change color
		if(x == 0)
		{
			setColor(col1);
		}

		// line in z
		pushBackVertex(Vec3(x, 0.0, -GRID_HALF_SIZE));
		pushBackVertex(Vec3(x, 0.0, GRID_HALF_SIZE));

		// if middle line change col so you can highlight the x-axis
		if(x == 0)
		{
			setColor(col2);
		}

		// line in the x
		pushBackVertex(Vec3(-GRID_HALF_SIZE, 0.0, x));
		pushBackVertex(Vec3(GRID_HALF_SIZE, 0.0, x));
	}

	// render
	end();
}

//==============================================================================
void DebugDrawer::drawSphere(F32 radius, I complexity)
{
#if 1
	Mat4 oldMMat = m_mMat;
	Mat4 oldVpMat = m_vpMat;

	setModelMatrix(m_mMat * Mat4(Vec4(0.0, 0.0, 0.0, 1.0),
		Mat3::getIdentity(), radius));

	begin(PrimitiveTopology::LINES);

	// Pre-calculate the sphere points5
	F32 fi = getPi<F32>() / complexity;

	Vec3 prev(1.0, 0.0, 0.0);
	for(F32 th = fi; th < getPi<F32>() * 2.0 + fi; th += fi)
	{
		Vec3 p = Mat3(Euler(0.0, th, 0.0)) * Vec3(1.0, 0.0, 0.0);

		for(F32 th2 = 0.0; th2 < getPi<F32>(); th2 += fi)
		{
			Mat3 rot(Euler(th2, 0.0, 0.0));

			Vec3 rotPrev = rot * prev;
			Vec3 rotP = rot * p;

			pushBackVertex(rotPrev);
			pushBackVertex(rotP);

			Mat3 rot2(Euler(0.0, 0.0, getPi<F32>() / 2));

			pushBackVertex(rot2 * rotPrev);
			pushBackVertex(rot2 * rotP);
		}

		prev = p;
	}

	end();

	m_mMat = oldMMat;
	m_vpMat = oldVpMat;
#endif
}

//==============================================================================
void DebugDrawer::drawCube(F32 size)
{
	Vec3 maxPos = Vec3(0.5 * size);
	Vec3 minPos = Vec3(-0.5 * size);

	Array<Vec3, 8> points = {{
		Vec3(maxPos.x(), maxPos.y(), maxPos.z()),  // right top front
		Vec3(minPos.x(), maxPos.y(), maxPos.z()),  // left top front
		Vec3(minPos.x(), minPos.y(), maxPos.z()),  // left bottom front
		Vec3(maxPos.x(), minPos.y(), maxPos.z()),  // right bottom front
		Vec3(maxPos.x(), maxPos.y(), minPos.z()),  // right top back
		Vec3(minPos.x(), maxPos.y(), minPos.z()),  // left top back
		Vec3(minPos.x(), minPos.y(), minPos.z()),  // left bottom back
		Vec3(maxPos.x(), minPos.y(), minPos.z())   // right bottom back
	}};

	static const Array<U32, 24> indeces = {{
		0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6,
		6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7}};

	begin(PrimitiveTopology::LINES);
		for(U32 id : indeces)
		{
			pushBackVertex(points[id]);
		}
	end();
}

//==============================================================================
// CollisionDebugDrawer                                                        =
//==============================================================================

//==============================================================================
void CollisionDebugDrawer::visit(const Sphere& sphere)
{
	m_dbg->setModelMatrix(
		Mat4(sphere.getCenter().xyz1(), Mat3::getIdentity(), 1.0));
	m_dbg->drawSphere(sphere.getRadius());
}

//==============================================================================
void CollisionDebugDrawer::visit(const Obb& obb)
{
	Mat4 scale(Mat4::getIdentity());
	scale(0, 0) = obb.getExtend().x();
	scale(1, 1) = obb.getExtend().y();
	scale(2, 2) = obb.getExtend().z();

	Mat4 tsl(Transform(obb.getCenter(), obb.getRotation(), 1.0));

	m_dbg->setModelMatrix(tsl * scale);
	m_dbg->drawCube(2.0);
}

//==============================================================================
void CollisionDebugDrawer::visit(const Plane& plane)
{
	Vec3 n = plane.getNormal().xyz();
	const F32& o = plane.getOffset();
	Quat q;
	q.setFrom2Vec3(Vec3(0.0, 0.0, 1.0), n);
	Mat3 rot(q);
	rot.rotateXAxis(getPi<F32>() / 2.0);
	Mat4 trf(Vec4(n * o, 1.0), rot);

	m_dbg->setModelMatrix(trf);
	m_dbg->drawGrid();
}

//==============================================================================
void CollisionDebugDrawer::visit(const LineSegment& ls)
{
	m_dbg->setModelMatrix(Mat4::getIdentity());
	m_dbg->begin(PrimitiveTopology::LINES);
	m_dbg->pushBackVertex(ls.getOrigin().xyz());
	m_dbg->pushBackVertex((ls.getOrigin() + ls.getDirection()).xyz());
	m_dbg->end();
}

//==============================================================================
void CollisionDebugDrawer::visit(const Aabb& aabb)
{
	Vec3 min = aabb.getMin().xyz();
	Vec3 max = aabb.getMax().xyz();

	Mat4 trf = Mat4::getIdentity();
	// Scale
	for(U32 i = 0; i < 3; ++i)
	{
		trf(i, i) = max[i] - min[i];
	}

	// Translation
	trf.setTranslationPart(Vec4((max + min) / 2.0, 1.0));

	m_dbg->setModelMatrix(trf);
	m_dbg->drawCube();
}

//==============================================================================
void CollisionDebugDrawer::visit(const Frustum& f)
{
	switch(f.getType())
	{
	case Frustum::Type::ORTHOGRAPHIC:
		visit(static_cast<const OrthographicFrustum&>(f).getObb());
		break;
	case Frustum::Type::PERSPECTIVE:
		{
			const PerspectiveFrustum& pf =
				static_cast<const PerspectiveFrustum&>(f);

			F32 camLen = pf.getFar();
			F32 tmp0 = camLen / tan((getPi<F32>() - pf.getFovX()) * 0.5)
				+ 0.001;
			F32 tmp1 = camLen * tan(pf.getFovY() * 0.5) + 0.001;

			Vec3 points[] = {
				Vec3(0.0, 0.0, 0.0), // 0: eye point
				Vec3(-tmp0, tmp1, -camLen), // 1: top left
				Vec3(-tmp0, -tmp1, -camLen), // 2: bottom left
				Vec3(tmp0, -tmp1, -camLen), // 3: bottom right
				Vec3(tmp0, tmp1, -camLen) // 4: top right
			};

			const U32 indeces[] = {0, 1, 0, 2, 0, 3, 0, 4, 1, 2, 2,
				3, 3, 4, 4, 1};

			m_dbg->begin(PrimitiveTopology::LINES);
			for(U32 i = 0; i < sizeof(indeces) / sizeof(U32); i++)
			{
				m_dbg->pushBackVertex(points[indeces[i]]);
			}
			m_dbg->end();
			break;
		}
	}
}

//==============================================================================
void CollisionDebugDrawer::visit(const CompoundShape& cs)
{
	CollisionDebugDrawer* self = this;
	Error err = cs.iterateShapes([&](const CollisionShape& a) -> Error
	{
		a.accept(*self);
		return ErrorCode::NONE;
	});
	(void) err;
}

//==============================================================================
void CollisionDebugDrawer::visit(const ConvexHullShape& hull)
{
	m_dbg->setModelMatrix(Mat4(hull.getTransform()));
	m_dbg->begin(PrimitiveTopology::LINES);
	const Vec4* points = hull.getPoints() + 1;
	const Vec4* end = hull.getPoints() + hull.getPointsCount();
	for(; points != end; ++points)
	{
		m_dbg->pushBackVertex(hull.getPoints()->xyz());
		m_dbg->pushBackVertex(points->xyz());
	}
	m_dbg->end();
}

//==============================================================================
// PhysicsDebugDrawer                                                          =
//==============================================================================
void PhysicsDebugDrawer::drawLines(
	const Vec3* lines,
	const U32 linesCount,
	const Vec4& color)
{
	m_dbg->begin(PrimitiveTopology::LINES);
	m_dbg->setColor(color);
	for(U i = 0; i < linesCount * 2; ++i)
	{
		m_dbg->pushBackVertex(lines[i]);
	}
	m_dbg->end();
}

//==============================================================================
// SceneDebugDrawer                                                            =
//==============================================================================

//==============================================================================
void SceneDebugDrawer::draw(SceneNode& node)
{
	MoveComponent* mv = node.tryGetComponent<MoveComponent>();
	if(mv)
	{
		m_dbg->setModelMatrix(Mat4(mv->getWorldTransform()));
	}
	else
	{
		m_dbg->setModelMatrix(Mat4::getIdentity());
	}

	FrustumComponent* fr = node.tryGetComponent<FrustumComponent>();
	if(fr)
	{
		draw(*fr);
	}

	Error err = node.iterateComponentsOfType<SpatialComponent>(
		[&](SpatialComponent& sp) -> Error
	{
		draw(sp);
		return ErrorCode::NONE;
	});

	PortalSectorComponent* ps = node.tryGetComponent<PortalSectorComponent>();
	if(ps)
	{
		draw(*ps);
	}

	(void)err;
}

//==============================================================================
void SceneDebugDrawer::draw(FrustumComponent& fr) const
{
	const Frustum& fs = fr.getFrustum();

	m_dbg->setColor(Vec3(1.0, 1.0, 0.0));
	CollisionDebugDrawer coldraw(m_dbg);
	fs.accept(coldraw);
}

//==============================================================================
void SceneDebugDrawer::draw(SpatialComponent& x) const
{
	if(!x.getVisibleByCamera())
	{
		return;
	}

	m_dbg->setColor(Vec3(1.0, 0.0, 1.0));
	CollisionDebugDrawer coldraw(m_dbg);
	x.getAabb().accept(coldraw);
}

//==============================================================================
void SceneDebugDrawer::draw(const PortalSectorComponent& c) const
{
	const SceneNode& node = c.getSceneNode();
	const PortalSectorBase& psnode = static_cast<const PortalSectorBase&>(node);

	m_dbg->setColor(Vec3(0.0, 0.0, 1.0));

	m_dbg->begin(PrimitiveTopology::LINES);
	const auto& verts = psnode.getVertices();
	ANKI_ASSERT((psnode.getVertexIndices().getSize() % 3) == 0);
	for(U i = 0; i < psnode.getVertexIndices().getSize(); i += 3)
	{
		I id0 = psnode.getVertexIndices()[i];
		I id1 = psnode.getVertexIndices()[i + 1];
		I id2 = psnode.getVertexIndices()[i + 2];

		m_dbg->pushBackVertex(verts[id0].xyz());
		m_dbg->pushBackVertex(verts[id1].xyz());

		m_dbg->pushBackVertex(verts[id1].xyz());
		m_dbg->pushBackVertex(verts[id2].xyz());

		m_dbg->pushBackVertex(verts[id2].xyz());
		m_dbg->pushBackVertex(verts[id0].xyz());
	}
	m_dbg->end();
}

//==============================================================================
void SceneDebugDrawer::drawPath(const Path& path) const
{
	/*const U count = path.getPoints().size();

	m_dbg->setColor(Vec3(1.0, 1.0, 0.0));

	m_dbg->begin();

	for(U i = 0; i < count - 1; i++)
	{
		m_dbg->pushBackVertex(path.getPoints()[i].getPosition());
		m_dbg->pushBackVertex(path.getPoints()[i + 1].getPosition());
	}

	m_dbg->end();*/
}

}  // end namespace anki
