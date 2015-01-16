#include <ros/ros.h>

#include <kobuki_control/kobuki_control.h>

//#define ODOM
#define VICON
//#define DB

#define rad2deg(x) ((x)*(180.0)/M_PI)
#define deg2rad(x) ((x)*M_PI/180.0)

ros::Publisher vel_pub;
ros::ServiceClient db_client;

double distance(double x0, double y0, double x1, double y1) {
	return sqrt((x1-x0)*(x1-x0)+(y1-y0)*(y1-y0));
}

// calculate z-axis rotation from Odom_Quaternion(z, w) -180~+180
double quaternion_to_enler(double z, double w) {
    double sqw, sqz;
    sqw = w*w;
    sqz = z*z;
    return atan2(2.0*(z*w),(-sqz + sqw));
}

//forward speed linear[m/s] rotational_speed angular[rad/s]
void pub_vel(double linear, double angular) {
    geometry_msgs::Twist vel;
    vel.linear.x = linear;
    vel.angular.z = angular;

    //publish the message
    vel_pub.publish(vel);
    return;
}

void vicon_sysCallback(const tms_msg_ss::vicon_data::ConstPtr& msg) {
	if (msg->subjectName == "kobuki") {
		//大域変数を更新
		v_pos_x = msg->translation.x;
		v_pos_y = msg->translation.y;
	    v_ori_th = msg->eulerXYZ[2] - 90;
	}
}

void odomCallback(const nav_msgs::Odometry::ConstPtr& msg) {
    double z, w;//quaternion
    double temp_th;
	//大域変数を更新
	pos_x = msg->pose.pose.position.x + x;
	pos_y = msg->pose.pose.position.y + y;

	z = msg->pose.pose.orientation.z;
	w = msg->pose.pose.orientation.w;
    temp_th = quaternion_to_enler(z, w);
    ori_th = rad2deg(temp_th) + th;
}

void control_base(double goal_dis, double goal_ang) {
	//移動前のオドメトリ情報を格納
	double ini_pos_x, ini_pos_y, ini_ori_th;
	double var_pos_x, var_pos_y, var_ori_th;//ini_th->quaternion to euler

//initialize variables
#ifdef ODOM
	ini_pos_x = pos_x;
	ini_pos_y = pos_y;
	ini_ori_th = ori_th;

	var_pos_x = pos_x;
	var_pos_y = pos_y;
	var_ori_th = ori_th;
#endif

#ifdef VICON
	ini_pos_x = v_pos_x;
	ini_pos_y = v_pos_y;
	ini_ori_th = v_ori_th;

	var_pos_x = v_pos_x;
	var_pos_y = v_pos_y;
	var_ori_th = v_ori_th;
#endif

#ifdef DB
	tms_msg_db::TmsdbGetData srv;
	srv.request.tmsdb.id = 2005; // ID : kobuki
	if (db_client.call(srv)) {
		ini_pos_x = srv.response.tmsdb[0].x;
		ini_pos_y = srv.response.tmsdb[0].y;
		ini_ori_th = srv.response.tmsdb[0].ry;

		var_pos_x = srv.response.tmsdb[0].x;
		var_pos_y = srv.response.tmsdb[0].y;
		var_ori_th = srv.response.tmsdb[0].ry;
	} else {
		ROS_ERROR("Failed to call service tms db get kobuki's data\n");
	}
#endif

	if (goal_ang > 0) { // rotate +
		while (1) {
			// 180度の境界線を越えるとき
			if (ini_ori_th+goal_ang > 180) {
				if ((-180-ini_ori_th <= var_ori_th && var_ori_th < goal_ang-360)
						|| (0 <= var_ori_th && var_ori_th <= 180-goal_ang)) {
					pub_vel(0.0, 0.523596);// 30deg/s
#ifdef ODOM
					var_ori_th = ori_th;
#endif
#ifdef VICON
					var_ori_th = v_ori_th;
#endif
#ifdef DB
					if (db_client.call(srv)) {
						var_ori_th = srv.response.tmsdb[0].ry;
					} else {
						ROS_ERROR("Failed to call service tms db get kobuki's data\n");
					}
#endif
				} else {
					pub_vel(0.0, 0.0);
					break;
				}
			} else {
				if (fabs(var_ori_th - ini_ori_th) < fabs(goal_ang)){
					// 1.+回転
					pub_vel(0.0, 0.523596); // 30deg/s
#ifdef ODOM
					var_ori_th = ori_th;
#endif
#ifdef VICON
					var_ori_th = v_ori_th;
#endif
#ifdef DB
					if (db_client.call(srv)) {
						var_ori_th = srv.response.tmsdb[0].ry;
					} else {
						ROS_ERROR("Failed to call service tms db get kobuki's data\n");
					}
#endif
				} else {
					pub_vel(0.0, 0.0);
					break;
				}
			}
		}
	} else { // rotate -
		while (1) {
			// 180度の境界線を越えるとき
			if(ini_ori_th+goal_ang < -180) {
				if ((-180-ini_ori_th <= var_ori_th && var_ori_th <= 0)
						|| (goal_ang+360 < var_ori_th && var_ori_th <= 180-ini_ori_th)) {
					pub_vel(0.0, -0.523596);//-(PI/2m)/s
#ifdef ODOM
					var_ori_th = ori_th;
#endif
#ifdef VICON
					var_ori_th = v_ori_th;
#endif
#ifdef DB
					if (db_client.call(srv)) {
						var_ori_th = srv.response.tmsdb[0].ry;
					} else {
						ROS_ERROR("Failed to call service tms db get kobuki's data\n");
					}
#endif
				} else {
					pub_vel(0.0, 0.0);
					break;
				}
			} else {
				if (fabs(var_ori_th - ini_ori_th) < fabs(goal_ang)) {
					// 1.+回転
					pub_vel(0.0, -0.523596); // -(PI/2m)/s
#ifdef ODOM
					var_ori_th = ori_th;
#endif
#ifdef VICON
					var_ori_th = v_ori_th;
#endif
#ifdef DB
					if (db_client.call(srv)) {
						var_ori_th = srv.response.tmsdb[0].ry;
					} else {
						ROS_ERROR("Failed to call service tms db get kobuki's data\n");
					}
#endif
				} else {
					pub_vel(0.0, 0.0);
					break;
				}
			}
		}
	}

	while (1) {
        if (distance(ini_pos_x, ini_pos_y, var_pos_x, var_pos_y) <  goal_dis/1000) { // 単位：mm -> m
			// 2.直進
			pub_vel(0.2, 0.0); // 0.25m/s
#ifdef ODOM
			var_pos_x = pos_x;
			var_pos_y = pos_y;
#endif
#ifdef VICON
			var_pos_x = v_pos_x;
			var_pos_y = v_pos_y;
#endif
#ifdef DB
					if (db_client.call(srv)) {
						var_pos_x = srv.response.tmsdb[0].x;
						var_pos_y = srv.response.tmsdb[0].y;
					} else {
						ROS_ERROR("Failed to call service tms db get kobuki's data\n");
					}
#endif
		} else {
			pub_vel(0.0, 0.0);
			break;
		}
	}
}

bool callback(tms_msg_rc::rc_robot_control::Request  &req,
		tms_msg_rc::rc_robot_control::Response &res) {
	switch (req.cmd) {
	case 0:
		///// control robot base1(絶対座標)
		// エラーチェック
		if (req.arg.size() != 3 || req.arg[0] < 0 || req.arg[0] > 8000
				|| req.arg[1] < 0 || req.arg[1] > 4500
				|| req.arg[2] < -180 || req.arg[2] > 180) {
			ROS_ERROR("case0 : An illegal arguments' type.\n");
			res.result = 0; //false
			return true;
		}

		// 現在地取得
		double current_x, current_y, current_th;
#ifdef ODOM
		current_x = pos_x;
		current_y = pos_y;
		current_th = ori_th;
		ROS_INFO("current_x=%fmm, current_y=%fmm, current_th=%fdeg\n", current_x, current_y, current_th);
#endif
#ifdef VICON
		current_x = v_pos_x;
		current_y = v_pos_y;
		current_th = v_ori_th;
		ROS_INFO("current_x=%fmm, current_y=%fmm, current_th=%fdeg\n", current_x, current_y, current_th);
#endif
#ifdef DB
		tms_msg_db::TmsdbGetData srv;
		srv.request.tmsdb.id = 2005; // ID : kobuki
		if (db_client.call(srv)) {
			current_x = srv.response.tmsdb[0].x;
			current_y = srv.response.tmsdb[0].y;
			current_th = srv.response.tmsdb[0].ry;
		} else {
			ROS_ERROR("Failed to call service tms db get kobuki's data\n");
			return false;
		}
		ROS_INFO("current_x=%fmm, current_y=%fmm, current_th=%fdeg\n", current_x, current_y, current_th);
#endif
		double goal_distance, goal_theta;
		goal_distance = distance(current_x, current_y, req.arg[0], req.arg[1]);
		goal_theta = req.arg[2] - current_th;
		if (goal_theta > 180.0) goal_theta = goal_theta - 360.0;
		else if (goal_theta < -180.0) goal_theta = goal_theta + 360.0;

		ROS_INFO("goal_dis=%fmm, goal_arg=%fdeg\n", goal_distance, goal_theta);
		control_base(goal_distance, goal_theta);
		res.result = 1;
		break;

	case 1:
		///// control robot base2(相対座標)
		// エラーチェック
		if (req.arg.size() != 2 || req.arg[0] < 0 || req.arg[0] > 9179
				|| req.arg[1] < -180 || req.arg[1] > 180) {
			ROS_ERROR("case0 : An illegal arguments' type.\n");
			res.result = 0; //false
			return true;}
		ROS_INFO("goal_dis=%fmm, goal_arg=%fdeg\n", req.arg[0], req.arg[1]);
		control_base(req.arg[0], req.arg[1]);
		res.result = 1;
		break;

	default :
		ROS_ERROR("An illegal command : %d\n", req.cmd);
		res.result = 0;
		return true;
	}
	return true;
}

int main(int argc, char **argv) {
	ros::init(argc, argv, "kobuki_control");
	ros::NodeHandle n;

	db_client = n.serviceClient<tms_msg_db::TmsdbGetData>("/tms_db_reader/dbreader");

#ifdef ODOM
	// kobukiの初期位置を格納
	tms_msg_db::TmsdbGetData srv;
	srv.request.tmsdb.id = 2005; // ID : kobuki
	if (db_client.call(srv)) {
		x = srv.response.tmsdb[0].x;
		y = srv.response.tmsdb[0].y;
		th = srv.response.tmsdb[0].ry;
	} else {
		ROS_ERROR("Failed to call service tms db get kobuki's data\n");
		return false;
	}
	ROS_INFO("kobuki's initial pos = [%fmm, %fmm, %fdeg]\n", x, y, th);
#endif

	ros::ServiceServer service = n.advertiseService("kobuki_control", callback);
	ROS_INFO("Ready to activate kobuki_control_node\n");

	ros::AsyncSpinner spinner(3);
	spinner.start();

	//現在地取得方法(vicon or odom)
	ros::Subscriber vicon_sub = n.subscribe("output", 10, vicon_sysCallback);
	ros::Subscriber odom_sub = n.subscribe("odom", 10, odomCallback);//Odometry情報取得

	vel_pub = n.advertise<geometry_msgs::Twist>("mobile_base/commands/velocity",1); //kobukiに速度を送る
	ros::waitForShutdown();

	return 0;
}
