#include "includes.h"

Movement g_movement{ };;

void Movement::RotateMovementAimware(CUserCmd* cmd, ang_t wish_angle, ang_t old_angles) {
	// aimware movement fix, reversed by ph4ge/senator for gucci
	if (old_angles.x != wish_angle.x || old_angles.y != wish_angle.y || old_angles.z != wish_angle.z) {
		vec3_t wish_forward, wish_right, wish_up, cmd_forward, cmd_right, cmd_up;

		auto viewangles = old_angles;
		auto movedata = vec3_t(cmd->m_forward_move,
			cmd->m_side_move,
			cmd->m_up_move);
		viewangles.normalize();

		if (viewangles.z != 0.f) {
			Player* pLocal = g_cl.m_local;

			if (pLocal && !(pLocal->m_fFlags() & FL_ONGROUND))
				movedata.y = 0.f;
		}

		wish_forward = wish_angle.ToVectors(&wish_right, &wish_up);
		cmd_forward = viewangles.ToVectors(&cmd_right, &cmd_up);

		float v8 = sqrt(wish_forward.x * wish_forward.x + wish_forward.y * wish_forward.y), v10 = sqrt(wish_right.x * wish_right.x + wish_right.y * wish_right.y), v12 = sqrt(wish_up.z * wish_up.z);

		vec3_t wish_forward_norm(1.0f / v8 * wish_forward.x, 1.0f / v8 * wish_forward.y, 0.f),
			wish_right_norm(1.0f / v10 * wish_right.x, 1.0f / v10 * wish_right.y, 0.f),
			wish_up_norm(0.f, 0.f, 1.0f / v12 * wish_up.z);

		float v14 = sqrt(cmd_forward.x * cmd_forward.x + cmd_forward.y * cmd_forward.y), v16 = sqrt(cmd_right.x * cmd_right.x + cmd_right.y * cmd_right.y), v18 = sqrt(cmd_up.z * cmd_up.z);

		vec3_t cmd_forward_norm(1.0f / v14 * cmd_forward.x, 1.0f / v14 * cmd_forward.y, 1.0f / v14 * 0.0f),
			cmd_right_norm(1.0f / v16 * cmd_right.x, 1.0f / v16 * cmd_right.y, 1.0f / v16 * 0.0f),
			cmd_up_norm(0.f, 0.f, 1.0f / v18 * cmd_up.z);

		float v22 = wish_forward_norm.x * movedata.x, v26 = wish_forward_norm.y * movedata.x, v28 = wish_forward_norm.z * movedata.x, v24 = wish_right_norm.x * movedata.y, v23 = wish_right_norm.y * movedata.y, v25 = wish_right_norm.z * movedata.y, v30 = wish_up_norm.x * movedata.z, v27 = wish_up_norm.z * movedata.z, v29 = wish_up_norm.y * movedata.z;

		cmd->m_forward_move = cmd_forward_norm.x * v24 + cmd_forward_norm.y * v23 + cmd_forward_norm.z * v25 + (cmd_forward_norm.x * v22 + cmd_forward_norm.y * v26 + cmd_forward_norm.z * v28) + (cmd_forward_norm.y * v30 + cmd_forward_norm.x * v29 + cmd_forward_norm.z * v27);
		cmd->m_side_move = cmd_right_norm.x * v24 + cmd_right_norm.y * v23 + cmd_right_norm.z * v25 + (cmd_right_norm.x * v22 + cmd_right_norm.y * v26 + cmd_right_norm.z * v28) + (cmd_right_norm.x * v29 + cmd_right_norm.y * v30 + cmd_right_norm.z * v27);
		cmd->m_up_move = cmd_up_norm.x * v23 + cmd_up_norm.y * v24 + cmd_up_norm.z * v25 + (cmd_up_norm.x * v26 + cmd_up_norm.y * v22 + cmd_up_norm.z * v28) + (cmd_up_norm.x * v30 + cmd_up_norm.y * v29 + cmd_up_norm.z * v27);

		cmd->m_forward_move = std::clamp(cmd->m_forward_move, -450.f, 450.f);
		cmd->m_side_move = std::clamp(cmd->m_side_move, -450.f, 450.f);
		cmd->m_up_move = std::clamp(cmd->m_up_move, -450.f, 450.f);
	}
}

void Movement::JumpRelated() {
	if (g_cl.m_local->m_MoveType() == MOVETYPE_NOCLIP)
		return;

	if ((g_cl.m_cmd->m_buttons & IN_JUMP) && !(g_cl.m_flags & FL_ONGROUND)) {
		// bhop.
		if (g_csgo.m_engine->IsInGame() || g_cl.m_processing)
			g_cl.m_cmd->m_buttons &= ~IN_JUMP;

		// duck jump ( crate jump ).
		if (g_menu.main.misc.airduck.get())
			g_cl.m_cmd->m_buttons |= IN_DUCK;
	}
}

void Movement::Strafe() {
	vec3_t velocity;
	float  delta, abs_delta, velocity_delta, correct;

	// don't strafe while we prolly want to jump scout..
	// if (g_movement.m_slow_motion)
	//    return;

	// don't strafe while noclipping or on ladders..
	if (g_cl.m_local->m_MoveType() == MOVETYPE_NOCLIP || g_cl.m_local->m_MoveType() == MOVETYPE_LADDER)
		return;

	// get networked velocity ( maybe absvelocity better here? ).
	// meh, should be predicted anyway? ill see.
	velocity = g_cl.m_local->m_vecAbsVelocity();

	// get the velocity len2d ( speed ).
	m_speed = velocity.length_2d();

	// compute the ideal strafe angle for our velocity.
	m_ideal = (m_speed > 0.f) ? math::rad_to_deg(std::asin(15.f / m_speed)) : 90.f;
	m_ideal2 = (m_speed > 0.f) ? math::rad_to_deg(std::asin(30.f / m_speed)) : 90.f;

	// some additional sanity.
	math::clamp(m_ideal, 0.f, 90.f);
	math::clamp(m_ideal2, 0.f, 90.f);

	// save entity bounds ( used much in circle-strafer ).
	m_mins = g_cl.m_local->m_vecMins();
	m_maxs = g_cl.m_local->m_vecMaxs();

	// save our origin
	m_origin = g_cl.m_local->m_vecOrigin();

	// disable strafing while pressing shift.
	if ((g_cl.m_buttons & IN_SPEED) || (g_cl.m_flags & FL_ONGROUND))
		return;

	// for changing direction.
	// we want to change strafe direction every call.
	m_switch_value *= -1.f;

	// for allign strafer.
	++m_strafe_index;

	if (g_csgo.m_engine->IsInGame() || g_cl.m_processing) {
		// took this idea from stacker, thank u !!!!
		enum EDirections {
			FORWARDS = 0,
			BACKWARDS = 180,
			LEFT = 90,
			RIGHT = -90,
			BACK_LEFT = 135,
			BACK_RIGHT = -135
		};

		float wish_dir{ };

		// get our key presses.
		bool holding_w = g_cl.m_buttons & IN_FORWARD;
		bool holding_a = g_cl.m_buttons & IN_MOVELEFT;
		bool holding_s = g_cl.m_buttons & IN_BACK;
		bool holding_d = g_cl.m_buttons & IN_MOVERIGHT;

		// move in the appropriate direction.
		if (holding_w) {
			//    forward left
			if (holding_a) {
				wish_dir += (EDirections::LEFT / 2);
			}
			//    forward right
			else if (holding_d) {
				wish_dir += (EDirections::RIGHT / 2);
			}
			//    forward
			else {
				wish_dir += EDirections::FORWARDS;
			}
		}
		else if (holding_s) {
			//    back left
			if (holding_a) {
				wish_dir += EDirections::BACK_LEFT;
			}
			//    back right
			else if (holding_d) {
				wish_dir += EDirections::BACK_RIGHT;
			}
			//    back
			else {
				wish_dir += EDirections::BACKWARDS;
			}

			g_cl.m_cmd->m_forward_move = 0;
		}
		else if (holding_a) {
			//    left
			wish_dir += EDirections::LEFT;
		}
		else if (holding_d) {
			//    right
			wish_dir += EDirections::RIGHT;
		}

		g_cl.m_strafe_angles.y += math::NormalizeYaw(wish_dir);
	}

	// cancel out any forwardmove values.
	g_cl.m_cmd->m_forward_move = 0.f;

	if (!g_csgo.m_engine->IsInGame() || !g_cl.m_processing)
		return;

	// get our viewangle change.
	delta = math::NormalizedAngle(g_cl.m_strafe_angles.y - m_old_yaw);

	// convert to absolute change.
	abs_delta = std::abs(delta);

	// save old yaw for next call.
	m_circle_yaw = m_old_yaw = g_cl.m_strafe_angles.y;

	// set strafe direction based on mouse direction change.
	if (delta > 0.f)
		g_cl.m_cmd->m_side_move = -450.f;

	else if (delta < 0.f)
		g_cl.m_cmd->m_side_move = 450.f;

	// we can accelerate more, because we strafed less then needed
	// or we got of track and need to be retracked.
	if (abs_delta <= m_ideal || abs_delta >= 30.f) {
		// compute angle of the direction we are traveling in.
		ang_t velocity_angle;
		math::VectorAngles(velocity, velocity_angle);

		// get the delta between our direction and where we are looking at.
		velocity_delta = math::NormalizeYaw(g_cl.m_strafe_angles.y - velocity_angle.y);

		// correct our strafe amongst the path of a circle.
		correct = m_ideal;

		if (velocity_delta <= correct || m_speed <= 15.f) {
			// not moving mouse, switch strafe every tick.
			if (-correct <= velocity_delta || m_speed <= 15.f) {
				g_cl.m_strafe_angles.y += (m_ideal * m_switch_value);
				g_cl.m_cmd->m_side_move = 450.f * m_switch_value;
			}

			else {
				g_cl.m_strafe_angles.y = velocity_angle.y - correct;
				g_cl.m_cmd->m_side_move = 450.f;
			}
		}

		else {
			g_cl.m_strafe_angles.y = velocity_angle.y + correct;
			g_cl.m_cmd->m_side_move = -450.f;
		}
	}
}

void Movement::DoPrespeed() {
	float   mod, min, max, step, strafe, time, angle;
	vec3_t  plane;

	// min and max values are based on 128 ticks.
	mod = g_csgo.m_globals->m_interval * 128.f;

	// scale min and max based on tickrate.
	min = 2.25f * mod;
	max = 5.f * mod;

	// compute ideal strafe angle for moving in a circle.
	strafe = m_ideal * 2.f;

	// clamp ideal strafe circle value to min and max step.
	math::clamp(strafe, min, max);

	// calculate time.
	time = 320.f / m_speed;

	// clamp time.
	math::clamp(time, 0.35f, 1.f);

	// init step.
	step = strafe;

	while (true) {
		// if we will not collide with an object or we wont accelerate from such a big step anymore then stop.
		if (!WillCollide(time, step) || max <= step)
			break;

		// if we will collide with an object with the current strafe step then increment step to prevent a collision.
		step += 0.2f;
	}

	if (step > max) {
		// reset step.
		step = strafe;

		while (true) {
			// if we will not collide with an object or we wont accelerate from such a big step anymore then stop.
			if (!WillCollide(time, step) || step <= -min)
				break;

			// if we will collide with an object with the current strafe step decrement step to prevent a collision.
			step -= 0.2f;
		}

		if (step < -min) {
			if (GetClosestPlane(plane)) {
				// grab the closest object normal
				// compute the angle of the normal
				// and push us away from the object.
				angle = math::rad_to_deg(std::atan2(plane.y, plane.x));
				step = -math::NormalizedAngle(m_circle_yaw - angle) * 0.1f;
			}
		}

		else
			step -= 0.2f;
	}

	else
		step += 0.2f;

	// add the computed step to the steps of the previous circle iterations.
	m_circle_yaw = math::NormalizedAngle(m_circle_yaw + step);

	// apply data to usercmd.
	g_cl.m_cmd->m_view_angles.y = m_circle_yaw;
	g_cl.m_cmd->m_side_move = (step >= 0.f) ? -450.f : 450.f;
	ang_t move_dir = g_cl.m_strafe_angles;
	RotateMovementAimware(g_cl.m_cmd, move_dir, g_cl.m_cmd->m_view_angles);
}

bool Movement::GetClosestPlane(vec3_t& plane) {
	CGameTrace            trace;
	CTraceFilterWorldOnly filter;
	vec3_t                start{ m_origin };
	float                 smallest{ 1.f };
	const float		      dist{ 75.f };

	// trace around us in a circle
	for (float step{ }; step <= math::pi_2; step += (math::pi / 10.f)) {
		// extend endpoint x units.
		vec3_t end = start;
		end.x += std::cos(step) * dist;
		end.y += std::sin(step) * dist;

		g_csgo.m_engine_trace->TraceRay(Ray(start, end, m_mins, m_maxs), CONTENTS_SOLID, &filter, &trace);

		// we found an object closer, then the previouly found object.
		if (trace.m_fraction < smallest) {
			// save the normal of the object.
			plane = trace.m_plane.m_normal;
			smallest = trace.m_fraction;
		}
	}

	// did we find any valid object?
	return smallest != 1.f && plane.z < 0.1f;
}

bool Movement::WillCollide(float time, float change) {
	struct PredictionData_t {
		vec3_t start;
		vec3_t end;
		vec3_t velocity;
		float  direction;
		bool   ground;
		float  predicted;
	};

	PredictionData_t      data;
	CGameTrace            trace;
	CTraceFilterWorldOnly filter;

	// set base data.
	data.ground = g_cl.m_flags & FL_ONGROUND;
	data.start = m_origin;
	data.end = m_origin;
	data.velocity = g_cl.m_local->m_vecVelocity();
	data.direction = math::rad_to_deg(std::atan2(data.velocity.y, data.velocity.x));

	for (data.predicted = 0.f; data.predicted < time; data.predicted += g_csgo.m_globals->m_interval) {
		// predict movement direction by adding the direction change.
		// make sure to normalize it, in case we go over the -180/180 turning point.
		data.direction = math::NormalizedAngle(data.direction + change);

		// pythagoras.
		float hyp = data.velocity.length_2d();

		// adjust velocity for new direction.
		data.velocity.x = std::cos(math::deg_to_rad(data.direction)) * hyp;
		data.velocity.y = std::sin(math::deg_to_rad(data.direction)) * hyp;

		// assume we bhop, set upwards impulse.
		if (data.ground)
			data.velocity.z = g_csgo.sv_jump_impulse->GetFloat();

		else
			data.velocity.z -= g_csgo.sv_gravity->GetFloat() * g_csgo.m_globals->m_interval;

		// we adjusted the velocity for our new direction.
		// see if we can move in this direction, predict our new origin if we were to travel at this velocity.
		data.end += (data.velocity * g_csgo.m_globals->m_interval);

		// trace
		g_csgo.m_engine_trace->TraceRay(Ray(data.start, data.end, m_mins, m_maxs), MASK_PLAYERSOLID, &filter, &trace);

		// check if we hit any objects.
		if (trace.m_fraction != 1.f && trace.m_plane.m_normal.z <= 0.9f)
			return true;
		if (trace.m_startsolid || trace.m_allsolid)
			return true;

		// adjust start and end point.
		data.start = data.end = trace.m_endpos;

		// move endpoint 2 units down, and re-trace.
		// do this to check if we are on th floor.
		g_csgo.m_engine_trace->TraceRay(Ray(data.start, data.end - vec3_t{ 0.f, 0.f, 2.f }, m_mins, m_maxs), MASK_PLAYERSOLID, &filter, &trace);

		// see if we moved the player into the ground for the next iteration.
		data.ground = trace.hit() && trace.m_plane.m_normal.z > 0.7f;
	}

	// the entire loop has ran
	// we did not hit shit.
	return false;
}

void Movement::FixMove(CUserCmd* cmd, const ang_t& wish_angles) {
	vec3_t  move, dir;
	float   delta, len;
	ang_t   move_angle;

	// convert movement to vector.
	move = { cmd->m_forward_move, cmd->m_side_move, 0.f };

	// get move length and ensure we're using a unit vector ( vector with length of 1 ).
	len = move.normalize();
	if (!len)
		return;

	// convert move to an angle.
	math::VectorAngles(move, move_angle);

	// calculate yaw delta.
	delta = (cmd->m_view_angles.y - wish_angles.y);

	// accumulate yaw delta.
	move_angle.y += delta;

	// calculate our new move direction.
	// dir = move_angle_forward * move_length
	math::AngleVectors(move_angle, &dir);

	// scale to og movement.
	dir *= len;

	// strip old flags.
	g_cl.m_cmd->m_buttons &= ~(IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT);

	// fix ladder and noclip.
	if (g_cl.m_local->m_MoveType() == MOVETYPE_LADDER) {
		// invert directon for up and down.
		if (cmd->m_view_angles.x >= 45.f && wish_angles.x < 45.f && std::abs(delta) <= 65.f)
			dir.x = -dir.x;

		// write to movement.
		cmd->m_forward_move = dir.x;
		cmd->m_side_move = dir.y;

		// set new button flags.
		if (cmd->m_forward_move > 200.f)
			cmd->m_buttons |= IN_FORWARD;

		else if (cmd->m_forward_move < -200.f)
			cmd->m_buttons |= IN_BACK;

		if (cmd->m_side_move > 200.f)
			cmd->m_buttons |= IN_MOVERIGHT;

		else if (cmd->m_side_move < -200.f)
			cmd->m_buttons |= IN_MOVELEFT;
	}

	// we are moving normally.
	else {
		// we must do this for pitch angles that are out of bounds.
		if (cmd->m_view_angles.x < -90.f || cmd->m_view_angles.x > 90.f)
			dir.x = -dir.x;

		// set move.
		cmd->m_forward_move = dir.x;
		cmd->m_side_move = dir.y;

		// set new button flags.
		if (cmd->m_forward_move > 0.f)
			cmd->m_buttons |= IN_FORWARD;

		else if (cmd->m_forward_move < 0.f)
			cmd->m_buttons |= IN_BACK;

		if (cmd->m_side_move > 0.f)
			cmd->m_buttons |= IN_MOVERIGHT;

		else if (cmd->m_side_move < 0.f)
			cmd->m_buttons |= IN_MOVELEFT;
	}
}

bool position_reset = false;
vec3_t position = { 0, 0, 0 };

__forceinline vec3_t vector_angles(const vec3_t& start, const vec3_t& end) {
	vec3_t delta = end - start;

	float magnitude = sqrtf(delta.x * delta.x + delta.y * delta.y);
	float pitch = atan2f(-delta.z, magnitude) * M_RADPI;
	float yaw = atan2f(delta.y, delta.x) * M_RADPI;

	vec3_t angle(pitch, yaw, 0.0f);
	return angle.clamp();
}

__forceinline vec3_t angle_vectors(const vec3_t& angles) {
	float sp, sy, cp, cy;
	sp = sinf(angles.x * M_PIRAD);
	cp = cosf(angles.x * M_PIRAD);
	sy = sinf(angles.y * M_PIRAD);
	cy = cosf(angles.y * M_PIRAD);

	return vec3_t{ cp * cy, cp * sy, -sp };
}

void Movement::AutoPeek() {
	// set to invert if we press the button.
	if (g_input.GetKeyState(g_menu.main.misc.autopeek.get())) {
		if (!position_reset)
		{
			position_reset = true;
			position = g_cl.m_local->GetAbsOrigin();
		}

		if (g_cl.m_old_shot)
			m_invert = true;

		if (!g_cl.m_pressing_move)
			m_invert = true;

		if (g_cl.m_pressing_move)
			m_invert = false;

		if (m_invert)
		{
			vec3_t direction = vector_angles(g_cl.m_local->GetAbsOrigin(), position);
			direction.y = g_cl.m_cmd->m_view_angles.y - direction.y;

			vec3_t new_move = angle_vectors(direction);
			new_move *= 450.f;

			new_move.x = std::clamp< float >(new_move.x, -450.f, 450.f);
			new_move.y = std::clamp< float >(new_move.y, -450.f, 450.f);

			g_cl.m_cmd->m_forward_move = new_move.x;
			g_cl.m_cmd->m_side_move = new_move.y;

			float distance = (g_cl.m_local->GetAbsOrigin() - position).length_2d_sqr();

			if (distance < 14 && g_cl.m_local->m_vecVelocity().length() < 10.f) {
				Movement::QuickStop();
				ang_t move_dir = g_cl.m_strafe_angles;
				RotateMovementAimware(g_cl.m_cmd, move_dir, g_cl.m_cmd->m_view_angles);
				m_invert = false;
			}
		}
	}
	else {
		position_reset = false;
		m_invert = false;
	}
	auto color = g_menu.main.misc.autopeek_color.get();
	if (g_input.GetKeyState(g_menu.main.misc.autopeek.get())) {
		render::sphere2(position, 13.f, 15.f, 1.f, color);
	}
}

void Movement::QuickStop() {
	// convert velocity to angular momentum.
	ang_t angle;
	math::VectorAngles(g_cl.m_local->m_vecVelocity(), angle);

	// get our current speed of travel.
	float speed = g_cl.m_local->m_vecVelocity().length();

	// fix direction by factoring in where we are looking.
	angle.y = g_cl.m_view_angles.y - angle.y;

	// convert corrected angle back to a direction.
	vec3_t direction;
	math::AngleVectors(angle, &direction);

	vec3_t stop = direction * -speed;

	if (g_cl.m_speed > 13.f) {
		g_cl.m_cmd->m_forward_move = stop.x;
		g_cl.m_cmd->m_side_move = stop.y;
	}
	else {
		g_cl.m_cmd->m_forward_move = 0.f;
		g_cl.m_cmd->m_side_move = 0.f;
	}
	ang_t move_dir = g_cl.m_strafe_angles;
	RotateMovementAimware(g_cl.m_cmd, move_dir, g_cl.m_cmd->m_view_angles);
}

void Movement::AutoStop() {
	bool can_stop = g_menu.main.aimbot.autostop_always_on.get();
	if ((can_stop) && g_aimbot.m_stop) {
		Movement::QuickStop();
		ang_t move_dir = g_cl.m_strafe_angles;
		RotateMovementAimware(g_cl.m_cmd, move_dir, g_cl.m_cmd->m_view_angles);
	}

	if (g_input.GetKeyState(g_menu.main.misc.airstop.get()) && !(g_cl.m_flags & FL_ONGROUND)) {
		Movement::QuickStop();
		ang_t move_dir = g_cl.m_strafe_angles;
		RotateMovementAimware(g_cl.m_cmd, move_dir, g_cl.m_cmd->m_view_angles);
	}
}

void Movement::FakeWalk() {
	vec3_t velocity{ g_cl.m_local->m_vecVelocity() };
	int    ticks{ }, max{ 16 };

	if (!g_input.GetKeyState(g_menu.main.misc.fakewalk.get()))
		return;

	if (!g_cl.m_local->GetGroundEntity())
		return;

	// user was running previously and abrubtly held the fakewalk key
	// we should quick-stop under this circumstance to hit the 0.22 flick
	// perfectly, and speed up our fakewalk after running even more.
	//if( g_cl.m_initial_flick ) {
	//	Movement::QuickStop( );
	//	return;
	//}

	// reference:
	// https://github.com/ValveSoftware/source-sdk-2013/blob/master/mp/src/game/shared/gamemovement.cpp#L1612

	// calculate friction.
	float friction = g_csgo.sv_friction->GetFloat() * g_cl.m_local->m_surfaceFriction();

	for (; ticks < g_cl.m_max_lag; ++ticks) {
		// calculate speed.
		float speed = velocity.length();

		// if too slow return.
		if (speed <= 0.1f)
			break;

		// bleed off some speed, but if we have less than the bleed, threshold, bleed the threshold amount.
		float control = std::max(speed, g_csgo.sv_stopspeed->GetFloat());

		// calculate the drop amount.
		float drop = control * friction * g_csgo.m_globals->m_interval;

		// scale the velocity.
		float newspeed = std::max(0.f, speed - drop);

		if (newspeed != speed) {
			// determine proportion of old speed we are using.
			newspeed /= speed;

			// adjust velocity according to proportion.
			velocity *= newspeed;
		}
	}

	// zero forwardmove and sidemove.
	if (ticks > ((max - 1) - g_csgo.m_cl->m_choked_commands) || !g_csgo.m_cl->m_choked_commands) {
		g_cl.m_cmd->m_forward_move = g_cl.m_cmd->m_side_move = 0.f;
	}
}

void Movement::ClampMovementSpeed(float speed) {
	float final_speed = speed;

	if (!g_cl.m_local || !g_cl.m_processing)
		return;

	if (!g_cl.m_cmd || !g_cl.m_processing)
		return;

	g_cl.m_cmd->m_buttons |= IN_SPEED;

	float squirt = std::sqrtf((g_cl.m_cmd->m_forward_move * g_cl.m_cmd->m_forward_move) + (g_cl.m_cmd->m_side_move * g_cl.m_cmd->m_side_move));

	if (squirt > speed) {
		float squirt2 = std::sqrtf((g_cl.m_cmd->m_forward_move * g_cl.m_cmd->m_forward_move) + (g_cl.m_cmd->m_side_move * g_cl.m_cmd->m_side_move));

		float cock1 = g_cl.m_cmd->m_forward_move / squirt2;
		float cock2 = g_cl.m_cmd->m_side_move / squirt2;

		auto Velocity = g_cl.m_local->m_vecVelocity().length_2d();

		g_cl.m_cmd->m_forward_move = cock1 * final_speed;
		g_cl.m_cmd->m_side_move = cock2 * final_speed;
	}
}