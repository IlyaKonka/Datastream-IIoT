package mapReduce;

import lombok.Getter;
import lombok.Setter;

@Getter
@Setter
public class QualityStatistic {
    public static final String GOOD = "good";
    public static final String DEFECT_1 = "defect1";
    public static final String DEFECT_2 = "defect2";

    private int good = 0;
    private int defect1 = 0;
    private int defect2 = 0;

    @Override
    public String toString() {
        return "good=" + good + ", defect1=" + defect1 + ", defect2=" + defect2;
    }
}
