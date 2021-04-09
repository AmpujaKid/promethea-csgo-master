#include "includes.h"

// here are the colors for the moneybot scheme:
// backgroud: 23, 25, 24
// pink highlights: 192, 92, 91
// light grey text: 168, 170, 170


void Form::draw() {
	// opacity should reach 1 in 500 milliseconds.
	constexpr float frequency = 1.f / 0.5f;

	// the increment / decrement per frame.
	float step = frequency * g_csgo.m_globals->m_frametime;

	// if open		-> increment
	// if closed	-> decrement
	m_open ? m_opacity += step : m_opacity -= step;

	// clamp the opacity.
	math::clamp(m_opacity, 0.f, 1.f);

	m_alpha = 255;
	if (!m_open)
		return;

	// get gui color.
	Color color = g_gui.m_color;
	color.a() = m_alpha;

	// background!
	render::rect_filled(m_x, m_y, m_width, m_height, { 23, 25, 24, 230 });

	// no border!
	render::rect(m_x, m_y, m_width, m_height, { 5, 5, 5, m_alpha });
	render::rect(m_x + 1, m_y + 1, m_width - 2, m_height - 2, { 192, 92, 91, 245 });
	render::rect(m_x + 2, m_y + 2, m_width - 4, m_height - 4, { 192, 92, 91, 245 });
	render::rect(m_x + 3, m_y + 3, m_width - 6, m_height - 6, { 192, 92, 91, 245 });
	render::rect(m_x + 4, m_y + 4, m_width - 8, m_height - 8, { 192, 92, 91, 245 });

	// draw tabs if we have any.
	if (!m_tabs.empty()) {
		Rect tabs_area = GetTabsRect();

		for (size_t i{}; i < m_tabs.size(); ++i) 
		{
			const auto& t = m_tabs[i];
			int font_width = render::menu.size(t->m_title).m_width;

			// text
			render::menu.string(tabs_area.x + (i * (tabs_area.w / m_tabs.size())) + 16, tabs_area.y,
				Color{ 168, 170, 170, m_alpha }, t->m_title);

			// active tab indicator
			render::rect_filled_fade(tabs_area.x + (i * (tabs_area.w / m_tabs.size())) + 10, tabs_area.y + 14,
				font_width + 11, 16, Color{ 23, 25, 24, m_alpha }, 0, 150);

			render::rect_filled(tabs_area.x + (i * (tabs_area.w / m_tabs.size())) + 10, tabs_area.y + 14,
				font_width + 11, 16,
				t == m_active_tab ? color : Color{ 23, 25, 24, 0 });

			render::rect_filled(tabs_area.x + (i * (tabs_area.w / m_tabs.size())) + 10, tabs_area.y + 16,
				font_width + 11, 16,
				t == m_active_tab ? Color{ 0, 0, 0, 215 } : Color{ 23, 25, 24, 0 });
		}

		// this tab has elements.
		if (!m_active_tab->m_elements.empty()) {
			// elements background and border.
			Rect el = GetElementsRect();

			render::rect_filled(el.x, el.y, el.w, el.h, { 23, 25, 24, m_alpha });
			render::rect(el.x, el.y, el.w, el.h, { 23, 25, 24, m_alpha });
			render::rect(el.x + 1, el.y + 1, el.w - 2, el.h - 2, { 23, 25, 24, m_alpha });

			// iterate elements to display.
			for (const auto& e : m_active_tab->m_elements) {

				// draw the active element last.
				if (!e || (m_active_element && e == m_active_element))
					continue;

				if (!e->m_show)
					continue;

				// this element we dont draw.
				if (!(e->m_flags & ElementFlags::DRAW))
					continue;

				e->draw();
			}

			// we still have to draw one last fucker.
			if (m_active_element && m_active_element->m_show)
				m_active_element->draw();
		}
	}
}