package states;

import clasteredServerClient.Packet;
import flash.Lib;
import flash.display.BlendMode;
import flixel.FlxCamera;
import flixel.FlxG;
import flixel.FlxSprite;
import flixel.FlxState;
import flixel.system.scaleModes.*;
import flixel.addons.nape.FlxNapeSpace;
import flixel.math.FlxMath;
import flixel.math.FlxRect;
import flixel.util.FlxColor;
import nape.geom.Vec2;
import openfl.Assets;
import clasteredServerClient.*;
import haxe.CallStack;

import flixel.input.keyboard.FlxKey;
import input.AbstractInputManager;
import input.AbstractInputManager.*;

using flixel.util.FlxSpriteUtil;
import flixel.system.macros.FlxMacroUtil;

/**
 * @author TiagoLr ( ~~~ProG4mr~~~ )
 */
class PlayState extends FlxState
{
	// Demo arena boundaries
	static var LEVEL_MIN_X;
	static var LEVEL_MAX_X;
	static var LEVEL_MIN_Y;
	static var LEVEL_MAX_Y;

	private var actions:AbstractInputManager = new AbstractInputManager();
	private var game:CSGame;
	private var orb:Npc;
	private var orbShadow:FlxSprite;
	private var hud:HUD;
	private var hudCam:FlxCamera;
	private var overlayCamera:FlxCamera;
	private var deadzoneOverlay:FlxSprite;

	///network attrs
	public var id:Int;
	public var npcs:Map<Int,Null<Npc>> = new Map<Int,Null<Npc>>(); 
	public var npc:Null<Npc> = null;
	public var npc_id:Int = 0;
	private var _angle:Float = 0;
	private static inline var _d_angle:Float = 2*3.14/180; //2 degree
	
	
	public var l:Lock = new Lock();
	public var connection:Null<Connection> = null;
	public var recv_loop:Bool = true;
	public var receiver:Null<Receiver> = null;
	public var packets:Array<Packet> = new Array<Packet>();
	
	public function connection_lost(){
		game.connection_lost();
	}
	///out messages
	private static inline var MSG_SET_DIRECTION:Int = 2;
	//in messages
	private static inline var MSG_NPC_UPDATE:Int=3;
	private static inline var MESSAGE_NPC_REMOVE:Int=5;
	private static inline var MSG_CLIENT_UPDATE:Int=6;
	///
	private var map:TiledLevel;
	
	override public function create():Void 
	{	
		FlxNapeSpace.init();
		
		LEVEL_MIN_X = -FlxG.stage.stageWidth / 2;
		LEVEL_MAX_X = FlxG.stage.stageWidth * 1.5;
		LEVEL_MIN_Y = -FlxG.stage.stageHeight / 2;
		LEVEL_MAX_Y = FlxG.stage.stageHeight * 1.5;
		
		super.create();
		game = cast FlxG.game;
		
		id = game.id;
		connection = game.connection;
		recv_loop = true;
		receiver = new Receiver(this);
		
//		FlxG.mouse.visible = false;
		
		FlxNapeSpace.velocityIterations = 5;
		FlxNapeSpace.positionIterations = 5;

		map = new TiledLevel();
		add(map);

//		createFloorTiles();
//		FlxNapeSpace.createWalls(LEVEL_MIN_X, LEVEL_MIN_Y, LEVEL_MAX_X, LEVEL_MAX_Y);
		// Walls border.
//		add(new FlxSprite(-FlxG.width / 2, -FlxG.height / 2, "assets/Border.png"));
		
		// Player orb
		//orbShadow = new FlxSprite(FlxG.width / 2, FlxG.height / 2, "assets/OrbShadow.png");
		//orbShadow.centerOffsets();
		//orbShadow.blend = BlendMode.MULTIPLY;
		
		//orb = new Npc(FlxG.width / 2, FlxG.height / 2, 1);
		
		//add(orbShadow);
		//add(orb);
		
		//orb.shadow = orbShadow;

		hud = new HUD();
		add(hud);

		// Camera Overlay
		deadzoneOverlay = new FlxSprite(-10000, -10000);
		deadzoneOverlay.makeGraphic(FlxG.width, FlxG.height, FlxColor.TRANSPARENT, true);
		//deadzoneOverlay.antialiasing = true;

		overlayCamera = new FlxCamera(0, 0, 640, 720);
		overlayCamera.bgColor = FlxColor.TRANSPARENT;
		overlayCamera.follow(deadzoneOverlay);
		FlxG.cameras.add(overlayCamera);
		add(deadzoneOverlay);
		
		
//		FlxG.camera.setScrollBoundsRect(LEVEL_MIN_X, LEVEL_MIN_Y,
//			LEVEL_MAX_X + Math.abs(LEVEL_MIN_X), LEVEL_MAX_Y + Math.abs(LEVEL_MIN_Y), true);
		//FlxG.camera.follow(orb, FlxCameraFollowStyle.NO_DEAD_ZONE);

/*
		hudCam = new FlxCamera(440, 0, hud.width, hud.height);
		hudCam.zoom = 1; // For 1/2 zoom out.
		hudCam.follow(hud.background, FlxCameraFollowStyle.NO_DEAD_ZONE);
		hudCam.alpha = .5;
		FlxG.cameras.add(hudCam);
*/
		
		//change to normal mapping
		var a = actions.addAction(GO_UP);
		a.addKey(FlxKey.W);
		a.addKey(FlxKey.UP);
		a=actions.addAction(GO_DOWN);
		a.addKey(FlxKey.S);
		a.addKey(FlxKey.DOWN);
		a=actions.addAction(GO_LEFT);
		a.addKey(FlxKey.A);
		a.addKey(FlxKey.LEFT);
		a=actions.addAction(GO_RIGHT);
		a.addKey(FlxKey.D);
		a.addKey(FlxKey.RIGHT);
		a=actions.addAction(ATTACK);
		a.addMouseKey(MouseID.MOUSE_LEFT);
	}
	
	function drawDeadzone() 
	{
		deadzoneOverlay.fill(FlxColor.TRANSPARENT);
		var dz:FlxRect = FlxG.camera.deadzone;
		if (dz == null)
			return;

		var lineLength:Int = 20;
		var lineStyle:LineStyle = { color: FlxColor.WHITE, thickness: 3 };
		
		// adjust points slightly so lines will be visible when at screen edges
		dz.x += lineStyle.thickness / 2;
		dz.width -= lineStyle.thickness;
		dz.y += lineStyle.thickness / 2;
		dz.height -= lineStyle.thickness;
		
		// Left Up Corner
		deadzoneOverlay.drawLine(dz.left, dz.top, dz.left + lineLength, dz.top, lineStyle);
		deadzoneOverlay.drawLine(dz.left, dz.top, dz.left, dz.top + lineLength, lineStyle);
		// Right Up Corner
		deadzoneOverlay.drawLine(dz.right, dz.top, dz.right - lineLength, dz.top, lineStyle);
		deadzoneOverlay.drawLine(dz.right, dz.top, dz.right, dz.top + lineLength, lineStyle);
		// Bottom Left Corner
		deadzoneOverlay.drawLine(dz.left, dz.bottom, dz.left + lineLength, dz.bottom, lineStyle);
		deadzoneOverlay.drawLine(dz.left, dz.bottom, dz.left, dz.bottom - lineLength, lineStyle);
		// Bottom Right Corner
		deadzoneOverlay.drawLine(dz.right, dz.bottom, dz.right - lineLength, dz.bottom, lineStyle);
		deadzoneOverlay.drawLine(dz.right, dz.bottom, dz.right, dz.bottom - lineLength, lineStyle);
	}
	
	public function setZoom(zoom:Float)
	{
		zoom = FlxMath.bound(zoom, 0.5, 4);
		FlxG.camera.zoom = zoom;
		
		var zoomDistDiffY;
		var zoomDistDiffX;
/*		
		if (zoom <= 1) 
		{
			zoomDistDiffX = Math.abs((LEVEL_MIN_X + LEVEL_MAX_X) - (LEVEL_MIN_X + LEVEL_MAX_X) / 1 + (1 - zoom));
			zoomDistDiffY = Math.abs((LEVEL_MIN_Y + LEVEL_MAX_Y) - (LEVEL_MIN_Y + LEVEL_MAX_Y) / 1 + (1 - zoom));
			zoomDistDiffX *= -.5;
			zoomDistDiffY *= -.5;
		}
		else
		{
			zoomDistDiffX = Math.abs((LEVEL_MIN_X + LEVEL_MAX_X) - (LEVEL_MIN_X + LEVEL_MAX_X) / zoom);
			zoomDistDiffY = Math.abs((LEVEL_MIN_Y + LEVEL_MAX_Y) - (LEVEL_MIN_Y + LEVEL_MAX_Y) / zoom);
			zoomDistDiffX *= .5;
			zoomDistDiffY *= .5;
		}
*/		
		zoomDistDiffX = ((LEVEL_MAX_X + (LEVEL_MIN_X))*(zoom-1));
		zoomDistDiffY = ((LEVEL_MAX_Y + (LEVEL_MIN_Y))*(zoom-1));
		
		if (zoom <= 1){
			zoomDistDiffX *= -1;
			zoomDistDiffY *= -1;
		}
		
		FlxG.camera.setScrollBoundsRect(
			LEVEL_MIN_X - zoomDistDiffX*0.5, 
			LEVEL_MIN_Y - zoomDistDiffY*0.5,
			LEVEL_MAX_X + Math.abs(LEVEL_MIN_X) + zoomDistDiffX,
			LEVEL_MAX_Y + Math.abs(LEVEL_MIN_Y) + zoomDistDiffY,
			false);
		
		FlxG.scaleMode.onMeasure(0,0);
		hud.updateZoom(FlxG.camera.zoom);
	}

	private function createFloorTiles() 
	{
		var	floorImg = Assets.getBitmapData("assets/FloorTexture.png");
		var imgWidth = floorImg.width;
		var imgHeight = floorImg.height;
		var i = LEVEL_MIN_X; 
		var j = LEVEL_MIN_Y; 
		
		while (i <= LEVEL_MAX_X)  
		{
			while (j <= LEVEL_MAX_Y)
			{
				add(new FlxSprite(i, j, floorImg));
				j += imgHeight;
			}
			i += imgWidth;
			j = LEVEL_MIN_Y;
		}
	}
	
	override public function update(elapsed:Float):Void 
	{	
		super.update(elapsed);
		
//		trace(elapsed);
//		trace(Sys.time());
//		trace(orb.x,orb.y);
		checkInput(elapsed);
		checkPackets(elapsed);
	}
	
	private function checkInput(elapsed:Float) {
		var speed = 200;
		var p:Packet = new Packet();
		var keys_changed:Bool = false;
		actions.update();
		if (actions.anyChanged([GO_UP, GO_DOWN, GO_LEFT, GO_RIGHT])){
			p.addChar(0);
			p.addChar(Math.round((actions.value(GO_RIGHT)-(actions.value(GO_LEFT)))*100));
			p.addChar(1);
			p.addChar(Math.round((actions.value(GO_DOWN)-(actions.value(GO_UP)))*100));
		}
		if (actions.anyChanged([ATTACK])){
			p.addChar(3);
			p.addChar(Math.round(actions.value(ATTACK)));
		}
		if(npc != null){
			var angle = Math.atan2(FlxG.mouse.y - npc.y, FlxG.mouse.x - npc.x);
//			trace(Math.abs(_angle-angle));
			if (Math.abs(_angle-angle) >= _d_angle){
				_angle = angle;
				p.addChar(2);
				p.addChar(Math.round(angle/3.14*120));	
			}
//			trace(angle / 3.14 * 180);
//			npc.angle = Math.round(angle / 3.14 * 180);
		}
		if (p.chanks.length>0){
			p.type = MSG_SET_DIRECTION;
//			trace(connection);
			connection.sendPacket(p);
//			trace("sended");
		}
		
		if (FlxG.keys.justPressed.U)
			setLerp(.1);
		if (FlxG.keys.justPressed.J)
			setLerp( -.1);
			
		if (FlxG.keys.justPressed.I)
			setLead(.5);
		if (FlxG.keys.justPressed.K)
			setLead( -.5);
			
		if (FlxG.keys.justPressed.O)
			setZoom(FlxG.camera.zoom + .1);
		if (FlxG.keys.justPressed.L)
			setZoom(FlxG.camera.zoom - .1);
			
		if (FlxG.keys.justPressed.M)
			FlxG.camera.shake();
		
	}
	
	private function checkPackets(elapsed:Float) {
		var p:Null<Packet> = null;
		do{
			l.lock();
				p = packets.pop();
			l.unlock();
			if (p!=null){
				switch p.type {
					case MSG_NPC_UPDATE:
						var n:Null<Npc> = npcs[p.chanks[0].i];
						if (n == null){
							n = new Npc(FlxG.camera.scroll.x-100, FlxG.camera.scroll.y-100, 0);//create object out of creen
							n.id = p.chanks[0].i;
							npcs[p.chanks[0].i] = n;
							add(n);
						}
						n.update_attributes(p);
					case MESSAGE_NPC_REMOVE:
						for (chank in p.chanks){
							var nid = chank.i;
							if (npc_id==nid){
								//player npc add screen you are died
							}else{
								var n:Null<Npc> = npcs[nid];
								if (n != null){
									npcs.remove(nid);
									remove(n);
									n.destroy();
									n = null;
								}
							}
						}
					case MSG_CLIENT_UPDATE:
						var i:Int=0;
						while(i<p.chanks.length-1){
							switch p.chanks[i].i {
								case 1:
									npc_id=p.chanks[++i].i;
									if (npcs[npc_id] == null){
										npcs[npc_id] = new Npc(0, 0, 0);
										npcs[npc_id].id = npc_id;
										add(npcs[npc_id]);
									}
									npc = npcs[npc_id];
									FlxG.camera.follow(npc, FlxCameraFollowStyle.NO_DEAD_ZONE);
									i++;
							}
						}
				}
			}
		}while(p != null);
	}
	
	private function setLead(lead:Float) 
	{
		var cam = FlxG.camera;
		cam.followLead.x += lead;
		cam.followLead.y += lead;
		
		if (cam.followLead.x < 0)
		{
			cam.followLead.x = 0;
			cam.followLead.y = 0;
		}
		
		hud.updateCamLead(cam.followLead.x);
	}
	
	private function setLerp(lerp:Float) 
	{
		var cam = FlxG.camera;
		cam.followLerp += lerp;
		cam.followLerp = Math.round(10 * cam.followLerp) / 10; // adding or subtracting .1 causes roundoff errors
		hud.updateCamLerp(cam.followLerp);
	}
	
	//for using custom actions, use with FlxG.autoPause = true;
	override public function onFocus():Void{
		
	} 
	
	override public function onFocusLost():Void{
		
	} 
	
}

@:enum
abstract ActionID(Int) from Int to Int{
	public static var fromStringMap(default, null):Map<String, ActionID>
		= FlxMacroUtil.buildMap("PlayState.ActionID");
		
	public static var toStringMap(default, null):Map<ActionID, String>
		= FlxMacroUtil.buildMap("PlayState.ActionID", true);

	var NONE = -1;	
	var GO_UP = 1;
	var GO_DOWN = 2;
	var GO_LEFT = 3;
	var GO_RIGHT = 4;
	var ATTACK = 5;
	
	@:from
	public static inline function fromString(s:String)
	{
		s = s.toUpperCase();
		return fromStringMap.exists(s) ? fromStringMap.get(s) : NONE;
	}
	
	@:to
	public inline function toString():String
	{
		return toStringMap.get(this);
	}
}