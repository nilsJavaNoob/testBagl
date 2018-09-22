public class House {
	Floor floors[];
	public House(int floorCount, int appartmentCount){
		floors  = new Floor[floorCount];
		for(int i =0; i< floorCount;i++){
			floors[i] = new Floor(i+1,appartmentCount);
		}
	}
	public String toString(){
		String result = "     House\n";
		result += "==================\n";
		for(Floor floor : floors){
			result += floor.toString();
		}
		result += "==================\n";
		return result;
	}
}//house