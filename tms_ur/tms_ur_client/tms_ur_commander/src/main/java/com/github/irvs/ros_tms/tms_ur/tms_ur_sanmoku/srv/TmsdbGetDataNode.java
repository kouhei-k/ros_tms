package com.github.irvs.ros_tms.tms_ur.tms_ur_sanmoku.srv;

import android.util.Log;

import com.github.irvs.ros_tms.tms_ur.tms_ur_sanmoku.data.TmsdbObject;

import org.ros.exception.RemoteException;
import org.ros.exception.RosRuntimeException;
import org.ros.exception.ServiceNotFoundException;
import org.ros.namespace.GraphName;
import org.ros.node.AbstractNodeMain;
import org.ros.node.ConnectedNode;
import org.ros.node.service.ServiceClient;
import org.ros.node.service.ServiceResponseListener;

import java.util.ArrayList;

import tms_msg_db.Tmsdb;
import tms_msg_db.TmsdbGetData;
import tms_msg_db.TmsdbGetDataRequest;
import tms_msg_db.TmsdbGetDataResponse;
import tms_msg_ts.ts_req;
import tms_msg_ts.ts_reqRequest;
import tms_msg_ts.ts_reqResponse;

public class TmsdbGetDataNode extends AbstractNodeMain {

	private ServiceClient<TmsdbGetDataRequest,TmsdbGetDataResponse> srvClient;
	private ServiceClient<ts_reqRequest, ts_reqResponse> tms_ts_client;

	private static final String className = "TmsdbGetDataNode";
	private int isCalled = 0;
	private int result = 0;

	//現状すべての必要な格納領域を用意している
	//が、この実装には問題があると思う

	//memo
	//dbに送るデータについて
	//id=id, state=1で送るとidテーブルからデータを引っ張ってこれる

	//単品情報の格納領域
	private TmsdbObject obj = null;

	//上位オブジェクトの格納領域(placeにあたる)
	private TmsdbObject src = null;

	//家具列の格納領域
	private ArrayList<TmsdbObject> furnitures = null;

	//物品列の格納領域
	private ArrayList<TmsdbObject> objects = null;


	//ゲッター
	public TmsdbObject getObject(){
		return obj;
	}

	public TmsdbObject getSrc(){
		return src;
	}

	public ArrayList<TmsdbObject> getFurnitureArray(){
		return furnitures;
	}

	public ArrayList<TmsdbObject> getObjectArray(){
		return objects;
	}

	@Override
	public GraphName getDefaultNodeName() {
		return GraphName.of("tms_ur_client/TmsdbGetDataNode");
	}

	@Override
	public void onStart(final ConnectedNode connectedNode) {
		try {
			srvClient = connectedNode.newServiceClient("/tms_db_reader",TmsdbGetData._TYPE);
			tms_ts_client = connectedNode.newServiceClient("/tms_ts_master", ts_req._TYPE);
		} catch (ServiceNotFoundException e) {
			throw new RosRuntimeException(e);
		}
	}

	//家具情報を取得(AR用)
	//これを実行すると家具情報+ロボット情報がfurnitureに入る
	public int sendFurniture(){
		Log.v(className, "getFurnitures");
		int cnt = 0;
		int sid = 100000;
		result = 0;
		isCalled = 0;
		TmsdbGetDataRequest request = srvClient.newMessage();
		request.getTmsdb().setType(TmsdbObject.TYPE._FURNITURE);
		request.getTmsdb().setId(sid);

		Log.v(className, "call");
		srvClient.call(request, new ServiceResponseListener<TmsdbGetDataResponse>(){
			@Override
			public void onSuccess(TmsdbGetDataResponse response) {
				Log.v(className, "onSuccess");
				isCalled = 1;
				furnitures = null;
				furnitures = new ArrayList<TmsdbObject>();
				for(int i=0;i<response.getTmsdb().size();i++){
					furnitures.add(new TmsdbObject(response.getTmsdb().get(i)));
				}
				result = response.getTmsdb().size();
			}
			@Override
			public void onFailure(RemoteException e) {
				Log.v(className, "onFailure");
				throw new RosRuntimeException(e);
			}
		});
		Log.v(className, "wait");
		while(isCalled!=1 && cnt<10){
			try {
				Thread.sleep(500);  // 500ms * 10 = 5sec
			} catch (InterruptedException e){}
			cnt++;
		}

		//irsがロボットのため追加
		cnt = 0;
		result = 0;
		isCalled = 0;
		request.getTmsdb().setType(TmsdbObject.TYPE._ROBOT);
		request.getTmsdb().setId(sid);
		Log.v(className, "call");
		srvClient.call(request, new ServiceResponseListener<TmsdbGetDataResponse>(){
			@Override
			public void onSuccess(TmsdbGetDataResponse response) {
				Log.v(className, "onSuccess");
				isCalled = 1;
				for(int i=0;i<response.getTmsdb().size();i++){
					furnitures.add(new TmsdbObject(response.getTmsdb().get(i)));
				}
				result = response.getTmsdb().size();
			}
			@Override
			public void onFailure(RemoteException e) {
				Log.v(className, "onFailure");
				throw new RosRuntimeException(e);
			}
		});
		Log.v(className, "wait");
		while(isCalled!=1 && cnt<10){
			try {
				Thread.sleep(500);  // 500ms * 10 = 5sec
			} catch (InterruptedException e){}
			cnt++;
		}

		return result;
	}


	//idを送信することで詳細データを取得(nowコレクション)
	//格納先はobj
	public int sendInfo(TmsdbObject data){
		Log.i("DB","ID"+data.getId());
		if(data.getId() == 0){
			obj = null;
			return -1;
		}
		Log.v(className, "getInfo");
		int cnt = 0;
		result = 0;
		isCalled = 0;//
//						//この時点でobjsの中は複数タグをand検索した物品情報になる
//
		TmsdbGetDataRequest request = srvClient.newMessage();
		request.getTmsdb().setId(data.getId());
		srvClient.call(request, new ServiceResponseListener<TmsdbGetDataResponse>(){
			@Override
			public void onSuccess(TmsdbGetDataResponse response) {
				Log.i(className, "onSuccess");
				Log.i(className,"size:"+response.getTmsdb().size());
				isCalled = 1;
				obj = null;
				if(response.getTmsdb().size()!=0/*!=null*/){
					if(response.getTmsdb().get(0).getType().equals(TmsdbObject.TYPE._OBJECT)){
						Log.i(className, "object");
						for(Tmsdb res:response.getTmsdb()){
							if(res.getState()==1){
								obj = new TmsdbObject(res);
								break;
							}
						}
					}
					else obj = new TmsdbObject(response.getTmsdb().get(0));
				}
				result = response.getTmsdb().size();
			}
			@Override
			public void onFailure(RemoteException e) {
				Log.i(className, "onFailure");
				throw new RosRuntimeException(e);
			}
		});
		Log.v(className, "wait");
		while(isCalled!=1 && cnt<10){
			try {
				Thread.sleep(500);  // 500ms * 10 = 5sec
			} catch (InterruptedException e){}
			cnt++;
		}
		return result;
	}

	//idを送信することで内部のオブジェクト列を取得
	//送信したidの情報
	//冗長なコーディングになっているのでplace検索機能が実装されたらなおす
	//これを呼ぶとsrcに入れたidの情報
	//objectsにplase==idの情報がはいるはず
	public int sendBelongObject(TmsdbObject data){
		Log.v(className, "getBelongObject");
		int cnt = 0;
		result = 0;
		isCalled = 0;
		TmsdbGetDataRequest request = srvClient.newMessage();
		objects = null;
		objects = new ArrayList<TmsdbObject>();
		request.getTmsdb().setPlace(data.getId());
		srvClient.call(request, new ServiceResponseListener<TmsdbGetDataResponse>(){
			@Override
			public void onSuccess(TmsdbGetDataResponse response) {
				Log.v(className, "onSuccess");
				isCalled = 1;
				if(response.getTmsdb().size()!=0){
					for(Tmsdb tmsdb:response.getTmsdb()){
						if(tmsdb.getState()==1){
							objects.add(new TmsdbObject(tmsdb));
						}
					}
				}
				result += objects.size();
			}
			@Override
			public void onFailure(RemoteException e) {
				Log.v(className, "onFailure");
				throw new RosRuntimeException(e);
			}
		});
		Log.v(className, "wait");
		while(isCalled!=1 && cnt<10){
			try {
				Thread.sleep(500);  // 500ms * 10 = 5sec
			} catch (InterruptedException e){}
			cnt++;
		}

		return result;
	}

	//String tagを送るとそのタグの入った物品情報を取ってきてobjectsに格納(defaultコレクション)
	public int sendTag(String tag){
		Log.v(className, "getBelongObject");
		int cnt = 0;
		result = 0;
		isCalled = 0;
		TmsdbGetDataRequest request = srvClient.newMessage();
		objects = null;
		objects = new ArrayList<TmsdbObject>();
		request.getTmsdb().setTag(tag);
		srvClient.call(request, new ServiceResponseListener<TmsdbGetDataResponse>(){
			@Override
			public void onSuccess(TmsdbGetDataResponse response) {
				Log.v(className, "onSuccess");
				isCalled = 1;
				if(response.getTmsdb().size()!=0){//String tagを送るとそのタグの入った物品情報を取ってきてobjectsに格納
					for(Tmsdb tmsdb:response.getTmsdb()){
						//							if(tmsdb.getState()==1){
						objects.add(new TmsdbObject(tmsdb));
						//							}
					}
				}
				result += objects.size();
			}
			@Override
			public void onFailure(RemoteException e) {
				Log.v(className, "onFailure");
				throw new RosRuntimeException(e);
			}
		});
		Log.v(className, "wait");
		while(isCalled!=1 && cnt<10){
			try {
				Thread.sleep(500);  // 500ms * 10 = 5sec
			} catch (InterruptedException e){}
			cnt++;
		}

		return 1;
	}

	public int sendCommand(int t_id,int r_id,int u_id,int p_id,int o_id){
		if(!(t_id>8000&&t_id<9000)){
			Log.v("TASK","invalid task_id:" + t_id);
			return 0;
		}
		ts_reqRequest req = tms_ts_client.newMessage();
		req.setRostime(0);
		req.setTaskId(t_id);
		req.setRobotId(r_id);
		req.setUserId(u_id);
		req.setPlaceId(p_id);
		req.setObjectId(o_id);
		req.setPriority(0);

		tms_ts_client.call(req, new ServiceResponseListener<ts_reqResponse>(){
			@Override
			public void onSuccess(ts_reqResponse response){
				Log.v("TASK","success");
			}
			@Override
			public void onFailure(RemoteException e){
				Log.v("TASK","Failure");
				throw new RosRuntimeException(e);
			}
		});
		return 1;
	}
}
