#include "includes.h"

namespace render {
	Font menu;;
	Font menu_shade;;
	Font name;;
	Font esp;;
	Font esp_small;;
	Font console;;
	Font hud;;
	Font cs;;
	Font indicator;;
	Font hitmarker_text;;
	Font arrows;;
}

void render::init( ) {
	menu       = Font( XOR( "Tahoma" ), 12, FW_NORMAL, FONTFLAG_NONE );
	menu_shade = Font( XOR( "Tahoma" ), 12, FW_NORMAL, FONTFLAG_DROPSHADOW );
	name	   = Font( XOR( "Visitor TT2 BRK" ), 9, FW_MEDIUM, FONTFLAG_OUTLINE | FONTFLAG_ANTIALIAS );
	esp        = Font( XOR( "Verdana" ), 12, FW_BOLD, FONTFLAG_DROPSHADOW );
	esp_small  = Font( XOR( "hooge 05_55 Cyr2" ), 9, FW_THIN, FONTFLAG_OUTLINE );
	console	   = Font( XOR( "Lucida Console" ), 10, FW_DONTCARE, FONTFLAG_DROPSHADOW );
	hud		   = Font( XOR( "Segoe UI" ), 14, FW_EXTRALIGHT, FONTFLAG_ANTIALIAS );
	cs		   = Font( XOR( "undefeated" ), 13, FW_MEDIUM, FONTFLAG_ANTIALIAS | FONTFLAG_DROPSHADOW );
	indicator  = Font( XOR( "Verdana" ), 27, FW_BOLD, FONTFLAG_ANTIALIAS | FONTFLAG_DROPSHADOW );
	hitmarker_text = Font( XOR( "Tahoma" ), 18, FW_NORMAL, FONTFLAG_DROPSHADOW | FONTFLAG_ANTIALIAS);
	arrows = Font( XOR( "4rrows" ), 65, FW_NORMAL, FONTFLAG_ANTIALIAS );
}

bool render::WorldToScreen( const vec3_t& world, vec2_t& screen ) {
	float w;

	const VMatrix& matrix = g_csgo.m_engine->WorldToScreenMatrix( );

	// check if it's in view first.
    // note - dex; w is below 0 when world position is around -90 / +90 from the player's camera on the y axis.
	w = matrix[ 3 ][ 0 ] * world.x + matrix[ 3 ][ 1 ] * world.y + matrix[ 3 ][ 2 ] * world.z + matrix[ 3 ][ 3 ];
	if( w < 0.001f )
		return false;

	// calculate x and y.
	screen.x = matrix[ 0 ][ 0 ] * world.x + matrix[ 0 ][ 1 ] * world.y + matrix[ 0 ][ 2 ] * world.z + matrix[ 0 ][ 3 ];
	screen.y = matrix[ 1 ][ 0 ] * world.x + matrix[ 1 ][ 1 ] * world.y + matrix[ 1 ][ 2 ] * world.z + matrix[ 1 ][ 3 ];

	screen /= w;

	// calculate screen position.
	screen.x = ( g_cl.m_width / 2 ) + ( screen.x * g_cl.m_width ) / 2;
	screen.y = ( g_cl.m_height / 2 ) - ( screen.y * g_cl.m_height ) / 2;

	return true;
}

void render::line( vec2_t v0, vec2_t v1, Color color ) {
	g_csgo.m_surface->DrawSetColor( color );
	g_csgo.m_surface->DrawLine( v0.x, v0.y, v1.x, v1.y );
}

void render::line( int x0, int y0, int x1, int y1, Color color ) {
	g_csgo.m_surface->DrawSetColor( color );
	g_csgo.m_surface->DrawLine( x0, y0, x1, y1 );
}

void render::rect( int x, int y, int w, int h, Color color ) {
	g_csgo.m_surface->DrawSetColor( color );
	g_csgo.m_surface->DrawOutlinedRect( x, y, x + w, y + h );
}

void render::rect_filled( int x, int y, int w, int h, Color color ) {
	g_csgo.m_surface->DrawSetColor( color );
	g_csgo.m_surface->DrawFilledRect( x, y, x + w, y + h );
}

void render::rect_filled_fade( int x, int y, int w, int h, Color color, int a1, int a2 ) {
	g_csgo.m_surface->DrawSetColor( color );
	g_csgo.m_surface->DrawFilledRectFade( x, y, x + w, y + h, a1, a2, false );
}

void render::rect_outlined( int x, int y, int w, int h, Color color, Color color2 ) {
	rect( x, y, w, h, color );
	rect( x - 1, y - 1, w + 2, h + 2, color2 );
	rect( x + 1, y + 1, w - 2, h - 2, color2 );
}

void render::circle( int x, int y, int radius, int segments, Color color ) {
	static int texture = g_csgo.m_surface->CreateNewTextureID( true );

	g_csgo.m_surface->DrawSetTextureRGBA( texture, &colors::white, 1, 1 );
	g_csgo.m_surface->DrawSetColor( color );
	g_csgo.m_surface->DrawSetTexture( texture );

	std::vector< Vertex > vertices{};

	float step = math::pi_2 / segments;
	for( float i{ 0.f }; i < math::pi_2; i += step )
		vertices.emplace_back( vec2_t{ x + ( radius * std::cos( i ) ), y + ( radius * std::sin( i ) ) } );

	g_csgo.m_surface->DrawTexturedPolygon( vertices.size( ), vertices.data( ) );
}

void render::FilledRectWorld(vec3_t center, vec2_t size, Color color, int angle) {
	vec3_t add_top_left, add_top_right,
		add_bot_left, add_bot_right;

	switch (angle) {
	case 0: // Z-AXIS
		add_top_left = vec3_t(-size.x, -size.y, 0);
		add_top_right = vec3_t(size.x, -size.y, 0);
		//
		add_bot_left = vec3_t(-size.x, size.y, 0);
		add_bot_right = vec3_t(size.x, size.y, 0);

		break;
	case 1: // Y-AXIS
		add_top_left = vec3_t(-size.x, 0, -size.y);
		add_top_right = vec3_t(size.x, 0, -size.y);
		//
		add_bot_left = vec3_t(-size.x, 0, size.y);
		add_bot_right = vec3_t(size.x, 0, size.y);

		break;
	case 2: // X-AXIS
		add_top_left = vec3_t(0, -size.y, -size.x);
		add_top_right = vec3_t(0, -size.y, size.x);
		//
		add_bot_left = vec3_t(0, size.y, -size.x);
		add_bot_right = vec3_t(0, size.y, size.x);

		break;
	}

	//top line
	vec3_t c_top_left = center + add_top_left;
	vec3_t c_top_right = center + add_top_right;

	//bottom line
	vec3_t c_bot_left = center + add_bot_left;
	vec3_t c_bot_right = center + add_bot_right;

	vec2_t top_left, top_right, bot_left, bot_right;
	if (WorldToScreen(c_top_left, top_left) &&
		WorldToScreen(c_top_right, top_right) &&
		WorldToScreen(c_bot_left, bot_left) &&
		WorldToScreen(c_bot_right, bot_right)) {

		static int tex = g_csgo.m_surface->CreateNewTextureID(true);

		g_csgo.m_surface->DrawSetTexture(tex);
		g_csgo.m_surface->DrawSetColor(color);

		Vertex vertices[4];
		vertices[0].init(vec2_t(top_left.x, top_left.y));
		vertices[1].init(vec2_t(top_right.x, top_right.y));
		vertices[2].init(vec2_t(bot_right.x, bot_right.y));
		vertices[3].init(vec2_t(bot_left.x, bot_left.y));

		g_csgo.m_surface->DrawTexturedPolygon(4, vertices, true);
	}
}

void render::gradient( int x, int y, int w, int h, Color color1, Color color2 ) {
	g_csgo.m_surface->DrawSetColor( color1 );
	g_csgo.m_surface->DrawFilledRectFade( x, y, x + w, y + h, color1.a( ), 0, false );

	g_csgo.m_surface->DrawSetColor( color2 );
	g_csgo.m_surface->DrawFilledRectFade( x, y, x + w, y + h, 0, color2.a( ), false );
}

void render::sphere( vec3_t origin, float radius, float angle, float scale, Color color ) {
	std::vector< Vertex > vertices{};

	// compute angle step for input radius and precision.
	float step = ( 1.f / radius ) + math::deg_to_rad( angle );

	for( float lat{}; lat < ( math::pi * scale ); lat += step ) {
		// reset.
		vertices.clear( );

		for( float lon{}; lon < math::pi_2; lon += step ) {
			vec3_t point{
				origin.x + ( radius * std::sin( lat ) * std::cos( lon ) ),
				origin.y + ( radius * std::sin( lat ) * std::sin( lon ) ),
				origin.z + ( radius * std::cos( lat ) )
			};

			vec2_t screen;
			if( WorldToScreen( point, screen ) )
				vertices.emplace_back( screen );
		}

		if( vertices.empty( ) )
			continue;

		g_csgo.m_surface->DrawSetColor( color );
		g_csgo.m_surface->DrawTexturedPolyLine( vertices.size( ), vertices.data( ) );
	}
}

Vertex render::RotateVertex( const vec2_t& p, const Vertex& v, float angle ) {
	// convert theta angle to sine and cosine representations.
	float c = std::cos( math::deg_to_rad( angle ) );
	float s = std::sin( math::deg_to_rad( angle ) );

	return {
        p.x + ( v.m_pos.x - p.x ) * c - ( v.m_pos.y - p.y ) * s, 
        p.y + ( v.m_pos.x - p.x ) * s + ( v.m_pos.y - p.y ) * c 
    };
}

void render::Font::string( int x, int y, Color color, const std::string& text,StringFlags_t flags /*= render::ALIGN_LEFT */ ) {
	wstring( x, y, color, util::MultiByteToWide( text ), flags );
}

void render::Font::string( int x, int y, Color color, const std::stringstream& text, StringFlags_t flags /*= render::ALIGN_LEFT */ ) {
	wstring( x, y, color, util::MultiByteToWide( text.str( ) ), flags );
}

void render::Font::wstring( int x, int y, Color color, const std::wstring& text, StringFlags_t flags /*= render::ALIGN_LEFT */ ) {
	int w, h;

	g_csgo.m_surface->GetTextSize( m_handle, text.c_str( ), w, h );
	g_csgo.m_surface->DrawSetTextFont( m_handle );
	g_csgo.m_surface->DrawSetTextColor( color );

	if( flags & ALIGN_RIGHT )
		x -= w;
	if( flags & render::ALIGN_CENTER )
		x -= w / 2;

	g_csgo.m_surface->DrawSetTextPos( x, y );
	g_csgo.m_surface->DrawPrintText( text.c_str( ), ( int )text.size( ) );
}

render::FontSize_t render::Font::size( const std::string& text ) {
	return wsize( util::MultiByteToWide( text ) );
}

render::FontSize_t render::Font::wsize( const std::wstring& text ) {
	FontSize_t res;
	g_csgo.m_surface->GetTextSize( m_handle, text.data( ), res.m_width, res.m_height );
	return res;
}