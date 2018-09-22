public class Floor {
	private int floorNumber =1;
	private Appartment appartments[];
	
	public Floor(int floorNumber, int appCount){
		this.floorNumber = floorNumber;
		
		appartments = new Appartment[appCount];
		for(int i =0;i<appCount;i++){
				appartments[i] = new Appartment();
		}
	}
	
	public  String toString(){
		String result = "|| Floor " + floorNumber + " ||\n";
		for(Appartment app : appartments){
			result += "\n" + app.toString(); 
		}
		return result;
	}
	
	
}//class