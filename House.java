public class House {
	Floor[] floors;
	
	public House(int floorCount,  int appartmentCount){
		floors = new Floor[floorCount];
		for(int i = 0;  i<floorCount; i++){
			floors[i] = new Floor(i+1,appartmentCount);
		}
	}
	public void settle(Owner owner){
		for(Floor floor:floors){
			Appartment app = floor.getFreeAppartment();
			//app exists
			if(app !=null){
				app.addOwner(owner);
				break;
			}
		}
	}//settle
	
	// tmp_house_vizual
	public String toString(){
		String result = "House\n";
		for(Floor floor : floors){
			result+=floor.toString()+"\n";
		}
		return result;
	}
}