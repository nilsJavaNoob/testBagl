public class House {
	
	Floor floors[];
	
	public House(int floorCount, int appartmentCount){

		System.out.println("Creating House ");
		NumberGenerator numberGenerator = new NumberGenerator();
		// numberGenerator = new NumberGenerator();

		NmbrGener nmbrGen = new NmbrGener();

		floors  = new Floor[floorCount];
		
		for(int i =0; i< floorCount;i++){

		floors[i] = new Floor(i+1,appartmentCount,numberGenerator);

			floors[i] = new Floor(i+1,appartmentCount,nmbrGen);

		}
	}
	public String toString(){
		String result = "     House\n";
		result += "==================\n";
		for(Floor floor : floors){
			result += floor.toString();
		}
		result += "==================\n";
		// if(numberGenerator != null)
			result+="\n GarbageCollector is absent today...";
		return result;
	}
}//house