typedef struct
{
    uint8_t counter;
    uint8_t tag;
    uint8_t useful;
} bank_entry;
typedef struct
{
    uint8_t counter;
} bimodal;

// custom taged predictors
typedef unsigned __int128 uint128_t;

// initialize the mask
uint128_t cgmask = (((uint128_t)1) << 80) - 1; // for global history
uint64_t cimask = (1 << 10) - 1;               // for index
uint64_t cpcmask = (1 << 10) - 1;              // for pc
uint64_t ctmask = (1 << 8) - 1;                // for tag
uint64_t cbimodalmask = (1 << 11) - 1;         // bimodal pc index

// cglobal_history initialization
uint128_t cglobal_history = 0;
bimodal bank0[2048] = {0};
bank_entry bank1[512] = {0};
bank_entry bank2[512] = {0};
bank_entry bank3[512] = {0};
bank_entry bank4[512] = {0};
bank_entry *prediction_entries[5] = {0}; // predictrion entry 0 is not used.

uint64_t mypow(uint64_t x, uint64_t n)
{
    uint64_t res = 1;
    while (n >= 1)
    {
        res = x * res;
        n--;
    }
    return res;
}
uint8_t predictor_idx = 0;     // provider component
uint8_t alt_predictor_idx = 0; // alt_predictor
uint8_t compute_tag(uint64_t bankn, uint64_t pc)
{
    uint128_t tgh = cglobal_history;
    uint8_t res = pc & ctmask;
    uint8_t csr1 = 0;
    uint8_t csr2 = 0;
    uint64_t his_len = mypow(2, bankn - 1) * 8 - 1;

    for (int i = his_len; i >= 0; i -= 8)
    {
        csr1 = tgh & 0xff;
        csr2 = tgh & 0xff;
        csr2 <<= 1;
        csr2 |= tgh & 0x1;
        csr2 = csr2 & ctmask;
        csr1 = csr1 & ctmask;
        tgh = tgh >> 8;

        res = res ^ csr1 ^ csr2;
    }
    return res;
}

uint64_t compute_index(uint64_t bankn, uint64_t pc)
{
    uint64_t res = pc & cpcmask;
    pc = pc >> 10;
    res = (res ^ (pc & cpcmask));
    uint64_t his_len = mypow(2, bankn - 1) * 10 - 1;
    uint64_t tgh = cglobal_history;
    for (int i = his_len; i >= 0; i -= 10)
    {
        res = res ^ (tgh & cimask);
        tgh = tgh >> 10;
    }
    return res;
}

uint64_t compute_bimodel_index(uint32_t pc)
{
    return pc & cbimodalmask;
}
uint8_t
custom_predict(uint32_t pc)
{

    bimodal entry0 = bank0[compute_bimodel_index(pc)];
    bank_entry *entry1 = &bank1[compute_index(1, pc)];
    bank_entry *entry2 = &bank2[compute_index(2, pc)];
    bank_entry *entry3 = &bank3[compute_index(3, pc)];
    bank_entry *entry4 = &bank4[compute_index(4, pc)];
    printf("custom_predict");fflush(stdout); 
    prediction_entries[1] = entry1;
    prediction_entries[2] = entry2;
    prediction_entries[3] = entry3;
    prediction_entries[4] = entry4;

    uint8_t s_tag1 = entry1->tag;
    uint8_t s_tag2 = entry2->tag;
    uint8_t s_tag3 = entry3->tag;
    uint8_t s_tag4 = entry4->tag;

    uint8_t c_tag1 = compute_tag(1, pc);
    uint8_t c_tag2 = compute_tag(2, pc);
    uint8_t c_tag3 = compute_tag(3, pc);
    uint8_t c_tag4 = compute_tag(4, pc);
    predictor_idx = 0;
    alt_predictor_idx = 0;
    if (c_tag4 == s_tag4)
    {
        predictor_idx = 4;
    }
    if (c_tag3 == s_tag3)
    {

        if (predictor_idx != 0)
        {
            alt_predictor_idx = 3;
        }
        else
        {
            predictor_idx = 3;
        }
    }
    if (c_tag2 == s_tag2)
    {

        if (predictor_idx != 0)
        {
            if (alt_predictor_idx == 0)
            {
                alt_predictor_idx = 2;
            }
        }
        else
        {
            predictor_idx = 2;
        }
    }
    if (c_tag1 == s_tag1)
    {

        if (predictor_idx != 0)
        {
            if (alt_predictor_idx == 0)
            {
                alt_predictor_idx = 1;
            }
        }
        else
        {
            predictor_idx = 1;
        }
    }

    printf("predict finished \n");fflush(stdout); 
    if (predictor_idx == 0)
        return (entry0.counter & 0x4);
    else
        return (prediction_entries[predictor_idx]->counter & 0x4);
}

uint8_t increment_3bit(uint8_t bit3)
{
    bit3++;
    if (bit3 > 7)
    {
        bit3 = 7;
    }
    return bit3;
}
uint8_t decrement_3bit(uint8_t bit3)
{
    if (bit3 == 0)
        return bit3;

    return --bit3;
}
uint8_t increment_2bit(uint8_t bit2)
{
    bit2++;
    if (bit2 > 3)
    {
        bit2 = 3;
    }
    return bit2;
}
uint8_t decrement_2bit(uint8_t bit2)
{
    if (bit2 == 0)
        return bit2;
    return --bit2;
}
void train_custom(uint32_t pc, uint8_t outcome)
{
    uint8_t predictor_prediction = 0;
    uint8_t alt_predictor_prediction = 0;
    printf("train customer started \n");fflush(stdout); 

    if (alt_predictor_idx != 0)
    {
        alt_predictor_prediction = prediction_entries[alt_predictor_idx]->counter & 0x4;
    }
    else
    {
        alt_predictor_prediction = bank0[compute_bimodel_index(pc)].counter & 0x4;
    }

    printf("train customer 210 \n");fflush(stdout); 

    if (predictor_idx != 0)
    {
        bank_entry prediction_entry = *prediction_entries[predictor_idx];
        predictor_prediction = prediction_entry.counter;
        if (alt_predictor_prediction != predictor_prediction)
        {
            printf("train customer 217 \n");fflush(stdout); 

            if (predictor_prediction == outcome)
            {
                printf("train customer 220 \n");fflush(stdout); 

                prediction_entry.useful = increment_2bit(prediction_entry.useful);
            }
            else
            {
                printf("train customer 225 \n");fflush(stdout); 

                prediction_entry.useful = decrement_2bit(prediction_entry.useful);
            }
        }

        if (predictor_prediction == outcome)
        {
            printf("train customer 232 \n");fflush(stdout); 

            prediction_entry.counter = increment_3bit(prediction_entry.counter);
        }
        else
        {
            printf("train customer 237 \n");fflush(stdout); 

            prediction_entry.counter = decrement_3bit(prediction_entry.counter);
            if (predictor_idx < 4)
            {
                int usefuls[5] = {-1};
                int num_zero_use = 0;
                for (int i = predictor_idx + 1; i < 5; i++)
                {
                    if (prediction_entries[i]->useful == 0)
                    {
                        usefuls[i] = 1;
                        num_zero_use++;
                    }
                };
                printf("train customer 248 \n");fflush(stdout); 

                if (num_zero_use == 0)
                {
                    for (int i = predictor_idx + 1; i < 5; i++)
                    {
                        prediction_entries[i]->useful -= 1;
                    }
                };
                if (num_zero_use == 1)
                {
                    for (int i = predictor_idx + 1; i < 5; i++)
                    {
                        if (prediction_entries[i]->useful == 0)
                        {
                            prediction_entries[i]->counter = WT;
                            prediction_entries[i]->tag = compute_tag(i, pc);
                        }
                    }
                }
                printf("train customer 269 \n");fflush(stdout); 

                if (num_zero_use > 1)
                {
                    int ir = rand();
                    double r = (double)ir / (double)RAND_MAX;
                    double threshold = 0.66;
                    int last_zero = 0;
                    int find_zero = 0;
                    printf("train customer 287\n");fflush(stdout); 
                    for (int i = predictor_idx + 1; i < 5; i++)
                    {
                        if (prediction_entries[i]->useful == 0)
                        {
                            if (r < threshold)
                            {
                                printf("train customer 294\n");fflush(stdout); 
                                // prediction_entries[i]->tag =  prediction_entries[i]->tag ;
                                prediction_entries[i]->counter = WT;     printf("train customer 296banki: %d pointer:%p\n", i,prediction_entries[i]);fflush(stdout); 
                                uint8_t tmp = compute_tag(i, pc);printf("train customer 297 banki: %d entry pointer:%p tmp:%d\n", i,prediction_entries[i],tmp);fflush(stdout); 
                                printf("tag pointer:%p, counter pointer:%p\n",&(prediction_entries[i]->tag),&(prediction_entries[i]->counter));
                                prediction_entries[i]->tag  = tmp;printf("train customer 298banki: %d pointer:%p\n %d", i,prediction_entries[i],tmp);fflush(stdout); 
                                find_zero = 1;
                                printf("train customer 299 banki: %d\n", i);fflush(stdout); 

                            }
                            else
                            {
                                printf("train customer 301\n");fflush(stdout); 

                                threshold = threshold + (1 - threshold) * 0.66;
                            }
                            last_zero = i;
                        }
                    }
                    printf("train customer 295 \n");fflush(stdout); 

                    if (find_zero == 0)
                    {
                        prediction_entries[last_zero]->counter = WT;
                        prediction_entries[last_zero]->tag = compute_tag(last_zero, pc);
                    }
                }
            }
        }
    }
    else
    {
        bimodal prediction_entry = bank0[compute_bimodel_index(pc)];

        if (predictor_prediction == outcome)
        {
            printf("train customer 316 \n");fflush(stdout); 

            prediction_entry.counter = increment_3bit(prediction_entry.counter);
        }
        else
        {
            printf("train customer 322 \n");fflush(stdout); 

            prediction_entry.counter = decrement_3bit(prediction_entry.counter);
            if (predictor_idx < 4)
            {
                int usefuls[5] = {-1};
                int num_zero_use = 0;
                for (int i = predictor_idx + 1; i < 5; i++)
                {
                    if (prediction_entries[i]->useful == 0)
                    {
                        usefuls[i] = 1;
                        num_zero_use++;
                    }
                };
                if (num_zero_use == 0)
                {
                    for (int i = predictor_idx + 1; i < 5; i++)
                    {
                        prediction_entries[i]->useful -= 1;
                    }
                };
                if (num_zero_use == 1)
                {
                    for (int i = predictor_idx + 1; i < 5; i++)
                    {
                        if (prediction_entries[i]->useful == 0)
                        {
                            prediction_entries[i]->counter = WT;
                            prediction_entries[i]->tag = compute_tag(i, pc);
                        }
                    }
                }
                if (num_zero_use > 1)
                {
                    int ir = rand();
                    double r = (double)ir / (double)RAND_MAX;
                    double threshold = 0.66;
                    int last_zero = 0;
                    int find_zero = 0;
                    for (int i = predictor_idx + 1; i < 5; i++)
                    {
                        if (prediction_entries[i]->useful == 0)
                        {
                            if (r < threshold)
                            {
                                prediction_entries[i]->counter = WT;
                                prediction_entries[i]->tag = compute_tag(i, pc);
                                find_zero = 1;
                            }
                            else
                            {
                                threshold = threshold + (1 - threshold) * 0.66;
                            }
                            last_zero = i;
                        }
                    }
                    if (find_zero == 0)
                    {
                        prediction_entries[last_zero]->counter = WT;
                        prediction_entries[last_zero]->tag = compute_tag(last_zero, pc);
                    }
                }
            }
        }
    }
    printf("train customer finished \n");fflush(stdout); 

    cglobal_history = ((cglobal_history << 1) | outcome) & cgmask;
}
uint64_t useful_reset_counter = 0;
#define RESET_PERIOD 262144
void reset_useful_counter()
{
    useful_reset_counter++;
    if (useful_reset_counter == RESET_PERIOD)
    {
        for (int i = 0; i < 512; i++)
        {
            bank1[i].useful = bank1[i].useful & 0x2;
            bank2[i].useful = bank2[i].useful & 0x2;
            bank3[i].useful = bank3[i].useful & 0x2;
            bank4[i].useful = bank4[i].useful & 0x2;
        }
    }
    if (useful_reset_counter == RESET_PERIOD * 2)
    {
        for (int i = 0; i < 512; i++)
        {
            bank1[i].useful = bank1[i].useful & 0x1;
            bank2[i].useful = bank2[i].useful & 0x1;
            bank3[i].useful = bank3[i].useful & 0x1;
            bank4[i].useful = bank4[i].useful & 0x1;
        }
    }
}