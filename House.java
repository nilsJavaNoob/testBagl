public class House {
	Floor floors[];
	public House(int floorCount){
		floors  = new Floor[floorCount];
		for(int i =0; i< floorCount;i++){
			floors[i] = new Floor(i+1);
		}
	}
	public String toString(){
		String result = "     House\n";
		result += "==================\n";
		for(Floor floor : floors){
			result +="|| "+ floor.toString()+" || \n";
		}
		result += "==================\n";
		return result;
	}
}//house