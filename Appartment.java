public class Appartment{
	
	private int appNmb = 0;
	private boolean status = true;
	
	public Appartment(int appNmb){
		this.appNmb = appNmb;
	}
	public boolean isFree(){
			return status;
	}
	
	public  void addOwnner(Owner owner){
		//
		status = false;
	}

}//class